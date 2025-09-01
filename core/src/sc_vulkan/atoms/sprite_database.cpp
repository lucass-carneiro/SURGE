#include "sc_vulkan/atoms/sprite_database.hpp"

#include "sc_logging.hpp"
#include "sc_random.hpp"
#include "sc_vulkan/sc_vulkan_command.hpp"
#include "sc_vulkan/sc_vulkan_images.hpp"
#include "sc_vulkan/sc_vulkan_malloc.hpp"
#include "sc_vulkan/sc_vulkan_pipelines.hpp"
#include "sc_vulkan/sc_vulkan_sync.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

// Per sprite shader data
struct SpriteData {
  alignas(16) glm::mat4 model_matrix{1.0};      // Model matrix
  alignas(16) glm::vec4 color_multiplier{1.0f}; // Color modifier
};

static constexpr auto sprite_data_size{sizeof(SpriteData)};

// Per database shader data
// TODO: This should be an UBO
struct PushConstants {
  glm::mat4 projection{1.0};                     // Global Projection matrix
  glm::mat4 view{1.0};                           // Global view matrix
  VkDeviceAddress sprite_data_buffer_address{0}; // GPU Address to sprite data buffer
};

static constexpr auto push_consants_size{sizeof(PushConstants)};

struct surge::renderer::vk::atom::sprite_database::SpriteDatabaseImpl {
  usize max_sprites{0}; // How many sprites we can add.

  Buffer cpu_data_buffer{};      // CPU staging buffer for sprite data.
  usize cpu_data_buffer_size{0}; // Num of sprite data elms. curr. stored in cpu_data_buffer

  Buffer gpu_data_buffer{};                  // GPU buffer with sprite data.
  usize gpu_data_buffer_size{0};             // Num of sprite data elms. sent to gpu_data_buffer
  VkDeviceAddress gpu_data_buffer_address{}; // Address of GPU buffer with sprite data.

  containers::mimalloc::Vector<Buffer> img_src_buffers{};        // CPU staging image data
  containers::mimalloc::Vector<AllocatedImage> img_dst_images{}; // GPU image data

  VkCommandBuffer cmd_buff{VK_NULL_HANDLE}; // Database cmd. buffer.
  VkCommandPool cmd_pool{VK_NULL_HANDLE};   // Database cmd. pool.

  VkFence image_transfer_fence{VK_NULL_HANDLE}; // Fence for sync. image transfers
  VkFence data_transfer_fence{VK_NULL_HANDLE};  // Fence for sync. data transfers

  containers::mimalloc::Vector<VkDescriptorImageInfo> img_desc_infos{}; // Image descriptor infos.
  containers::mimalloc::Vector<VkSampler> img_samplers{};               // Image sprite samplers.

  VkDescriptorSetLayout img_desc_layout{VK_NULL_HANDLE}; // Image descriptor layout
  VkDescriptorSet img_desc_set{VK_NULL_HANDLE};          // Image descriptor set

  VkPipelineLayout pipeline_layout{VK_NULL_HANDLE}; // Vulkan pipeline layout for the database.
  VkPipeline pipeline{VK_NULL_HANDLE};              // Vulkan pipeline for the database.
};

template <surge::usize dim> struct RandomImageData {
  std::array<surge::u32, dim * dim> colors{};
  VkExtent3D img_extent{dim, dim, 1};
  surge::usize total_image_size{4 * img_extent.width * img_extent.height * img_extent.depth};
};

template <surge::usize dim> static inline auto gen_random_img() -> RandomImageData<dim> {
  using namespace surge;

  RandomImageData<dim> image{};

  static random::Xoshiro128 rng{random::Xoshiro128::State{{31, 47, 79, 113}}};

  for (usize x = 0; x < dim; x++) {
    for (usize y = 0; y < dim; y++) {
      const auto color_1{glm::packUnorm4x8(glm::vec4{rng.next_float_in_range_inc(0.0, 1.0),
                                                     rng.next_float_in_range_inc(0.0, 1.0),
                                                     rng.next_float_in_range_inc(0.0, 1.0), 1.0f})};

      const auto color_2{glm::packUnorm4x8(glm::vec4{rng.next_float_in_range_inc(0.0, 1.0),
                                                     rng.next_float_in_range_inc(0.0, 1.0),
                                                     rng.next_float_in_range_inc(0.0, 1.0), 1.0f})};

      image.colors[y * dim + x] = ((x % 2) ^ (y % 2)) ? color_1 : color_2;
    }
  }

  return image;
}

auto surge::renderer::vk::atom::sprite_database::make_ortho_projection(const glm::vec2 &dims)
    -> glm::mat4 {
  return glm::ortho(0.0f, dims[0], 0.0f, dims[1], 1.0f, 0.0f);
}

auto surge::renderer::vk::atom::sprite_database::make_view(const glm::vec2 &eye) -> glm::mat4 {
  return glm::lookAt(glm::vec3{eye, 1.0f}, glm::vec3{eye, 0.0f}, glm::vec3{0.0f, 1.0f, 0.0f});
}

auto surge::renderer::vk::atom::sprite_database::make_model_matrix(const glm::vec2 &position,
                                                                   const glm::vec2 &scale, float z)
    -> glm::mat4 {
  const auto mv{glm::vec3{position, z}};
  const auto sc{glm::vec3{scale, 1.0f}};
  return glm::scale(glm::translate(glm::mat4{1.0f}, mv), sc);
}

auto surge::renderer::vk::atom::sprite_database::create(Context ctx, const CreateInfo &ci,
                                                        GrowableDescriptorAllocator &desc_alloc)
    -> Result<SpriteDatabase> {
  log_info("Creating sprite database");

  // Allocate object handle
  auto database{
      static_cast<SpriteDatabase>(allocators::mimalloc::malloc(sizeof(SpriteDatabaseImpl)))};

  if (database == nullptr) {
    log_error("Unable to allocate sprite database");
    return Err{vk_atom_sprite_database_init};
  }

  new (database) SpriteDatabaseImpl();

  // Database handle as void*, used for printing
  const auto dbh{static_cast<void *>(database)};

  // Basic initialization
  database->max_sprites = ci.max_sprites;

  // Vector reserving
  database->img_src_buffers.reserve(database->max_sprites);
  database->img_dst_images.reserve(database->max_sprites);
  database->img_desc_infos.reserve(database->max_sprites);
  database->img_samplers.reserve(database->max_sprites);

  // Data buffers
  {
    log_info("Creating sprite database {} data buffers", dbh);

    const auto buffer_size{database->max_sprites * sprite_data_size};

    // CPU buffer
    // We call VMA directly, instead of using a wrapper because we need custom flags
    VkBufferCreateInfo cpu_buffer_ci{};
    cpu_buffer_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    cpu_buffer_ci.pNext = nullptr;
    cpu_buffer_ci.size = buffer_size;
    cpu_buffer_ci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo staging_ai{};
    staging_ai.usage = VMA_MEMORY_USAGE_AUTO;
    staging_ai.flags
        = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    const auto result{vmaCreateBuffer(
        ctx->allocator, &cpu_buffer_ci, &staging_ai, &database->cpu_data_buffer.buffer,
        &database->cpu_data_buffer.allocation, &database->cpu_data_buffer.info)};

    if (result != VK_SUCCESS) {
      log_error("Unable to create sprite database {} CPU data buffer", dbh);
      return Err{vk_buffer_allocation};
    }

    // GPU buffers
    const auto gpu_buffer{create_buffer(ctx, buffer_size,
                                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
                                            | VK_BUFFER_USAGE_TRANSFER_DST_BIT
                                            | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                        VMA_MEMORY_USAGE_GPU_ONLY)};

    if (!gpu_buffer) {
      log_error("Unable to create sprite database {} GPU data buffer", dbh);
      return Err{gpu_buffer.error()};
    }

    database->gpu_data_buffer = *gpu_buffer;

    // Get GPU address
    VkBufferDeviceAddressInfo gpu_buffer_address_info{};
    gpu_buffer_address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    gpu_buffer_address_info.buffer = database->gpu_data_buffer.buffer;

    database->gpu_data_buffer_address
        = vkGetBufferDeviceAddress(ctx->device, &gpu_buffer_address_info);
  }

  // Cmd pool
  {
    log_info("Creating sprite database {} command pool", dbh);

    auto cmd_pool_create_info{command_pool_create_info(
        ctx->q_handles.transfer_idx, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)};

    const auto result{vkCreateCommandPool(ctx->device, &cmd_pool_create_info, get_alloc_callbacks(),
                                          &database->cmd_pool)};

    if (result != VK_SUCCESS) {
      log_error("Unable to allocate sprite database {} command pool", dbh);
      return Err{vk_cmd_pool_creation};
    }
  }

  // Command buffer
  {
    log_info("Creating sprite database {} command buffer", dbh);

    const auto cmd_buffer_info{command_buffer_alloc_info(database->cmd_pool, 1)};

    const auto result{vkAllocateCommandBuffers(ctx->device, &cmd_buffer_info, &database->cmd_buff)};

    if (result != VK_SUCCESS) {
      log_error("Unable to allocate sprite database {} command buffer", dbh);
      return Err{vk_cmd_buffer_creation};
    }
  }

  // Image transfer fence
  {
    log_info("Creating sprite database {} image transfer fence", dbh);

    const auto fence_ci{fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT)};
    const auto result{vkCreateFence(ctx->device, &fence_ci, get_alloc_callbacks(),
                                    &database->image_transfer_fence)};

    if (result != VK_SUCCESS) {
      log_error("Unable to create sprite database {} image transfer fence", dbh);
      return Err{vk_fence_creation};
    }
  }

  // Data transfer fence
  {
    log_info("Creating sprite database {} data transfer fence", dbh);

    const auto fence_ci{fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT)};
    const auto result{vkCreateFence(ctx->device, &fence_ci, get_alloc_callbacks(),
                                    &database->data_transfer_fence)};

    if (result != VK_SUCCESS) {
      log_error("Unable to create sprite database {} data transfer fence", dbh);
      return Err{vk_fence_creation};
    }
  }

  // Texture samplers
  // TODO: choose types of filters. Do mip maps
  {
    log_info("Creating sprite database {} texture samplers", dbh);

    VkSamplerCreateInfo texture_sampler{};
    texture_sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    texture_sampler.pNext = nullptr;
    texture_sampler.magFilter = VK_FILTER_NEAREST;
    texture_sampler.minFilter = VK_FILTER_NEAREST;

    for (usize i = 0; i < database->max_sprites; i++) {
      VkSampler sampler{};
      const auto result{
          vkCreateSampler(ctx->device, &texture_sampler, get_alloc_callbacks(), &sampler)};

      if (result != VK_SUCCESS) {
        log_error("Unable to create sprite database texture sampler:");
        return Err{vk_sampler_creation};
      }

      database->img_samplers.push_back(sampler);
    }
  }

  // Descriptor set layout
  {
    log_info("Creating sprite database {} descriptor set layout", dbh);

    VkDescriptorSetLayoutBinding texture_descriptor_binding{};
    texture_descriptor_binding.binding = 0;
    texture_descriptor_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texture_descriptor_binding.descriptorCount = static_cast<u32>(database->max_sprites);
    texture_descriptor_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    texture_descriptor_binding.pImmutableSamplers = database->img_samplers.data();

    std::array<VkDescriptorBindingFlags, 1> texture_binding_flags{
        VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT
        | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT};

    VkDescriptorSetLayoutBindingFlagsCreateInfo texture_descriptor_set_layout_binding_flags{};
    texture_descriptor_set_layout_binding_flags.sType
        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    texture_descriptor_set_layout_binding_flags.pNext = nullptr;
    texture_descriptor_set_layout_binding_flags.bindingCount
        = static_cast<u32>(texture_binding_flags.size());
    texture_descriptor_set_layout_binding_flags.pBindingFlags = texture_binding_flags.data();

    VkDescriptorSetLayoutCreateInfo texture_descriptor_set_layout_info{};
    texture_descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    texture_descriptor_set_layout_info.pNext = &texture_descriptor_set_layout_binding_flags;
    texture_descriptor_set_layout_info.flags = 0;
    texture_descriptor_set_layout_info.bindingCount = 1;
    texture_descriptor_set_layout_info.pBindings = &texture_descriptor_binding;

    const auto result{vkCreateDescriptorSetLayout(ctx->device, &texture_descriptor_set_layout_info,
                                                  get_alloc_callbacks(),
                                                  &database->img_desc_layout)};

    if (result != VK_SUCCESS) {
      log_error("Unable to create sprite database texture descriptor set layout");
      return Err{vk_descriptor_set_layout_build};
    }
  }

  // Descriptor set
  {
    log_info("Creating sprite database {} descriptor set", dbh);

    std::array<u32, 1> descriptor_counts{{static_cast<u32>(database->max_sprites)}};

    VkDescriptorSetVariableDescriptorCountAllocateInfo variable_descriptor_count_info{};
    variable_descriptor_count_info.sType
        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
    variable_descriptor_count_info.pNext = nullptr;
    variable_descriptor_count_info.descriptorSetCount = static_cast<u32>(descriptor_counts.size());
    variable_descriptor_count_info.pDescriptorCounts = descriptor_counts.data();

    const auto sprite_texture_set{
        desc_alloc.allocate(ctx, database->img_desc_layout, &variable_descriptor_count_info)};

    if (!sprite_texture_set) {
      log_error("Unable to allocate sprite database texture descriptor set");
      return Err{sprite_texture_set.error()};
    }

    database->img_desc_set = *sprite_texture_set;
  }

  // Shaders
  log_info("Loading sprite database {} shaders", dbh);

  const auto vertex_shader{load_shader_module(ctx, "shaders/sprite_database.vert.spv")};
  if (!vertex_shader) {
    log_error("Unable to load sprite database vertex shader");
    return Err{vertex_shader.error()};
  }

  const auto fragment_shader{load_shader_module(ctx, "shaders/sprite_database.frag.spv")};
  if (!fragment_shader) {
    log_error("Unable to load sprite database fragment shader");
    return Err{fragment_shader.error()};
  }

  // Pipeline layout
  {
    log_info("Creating sprite database {} pipeline layout", dbh);

    VkPushConstantRange push_constant_range{};
    push_constant_range.offset = 0;
    push_constant_range.size = push_consants_size;
    push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo pipeline_layout_ci{};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.pNext = nullptr;

    pipeline_layout_ci.pPushConstantRanges = &push_constant_range;
    pipeline_layout_ci.pushConstantRangeCount = 1;

    pipeline_layout_ci.pSetLayouts = &database->img_desc_layout;
    pipeline_layout_ci.setLayoutCount = 1;

    const auto result{vkCreatePipelineLayout(ctx->device, &pipeline_layout_ci,
                                             get_alloc_callbacks(), &database->pipeline_layout)};

    if (result != VK_SUCCESS) {
      log_error("Unable create compute pipeline layout:");
      return Err{vk_pipeline_layout_create};
    }
  }

  // Pipeline
  {
    log_info("Creating sprite database {} pipeline", dbh);

    GraphicsPipelineBuilder gpl_builder{};
    gpl_builder.set_layout(database->pipeline_layout);
    gpl_builder.set_shaders(*vertex_shader, *fragment_shader);
    gpl_builder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    gpl_builder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    gpl_builder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    gpl_builder.set_multisampling_none();

    switch (ci.blending_mode) {
    case BlendingMode::none:
      gpl_builder.set_blending_none();
      break;
    case BlendingMode::additive:
      gpl_builder.set_blending_additive();
      break;
    case BlendingMode::alpha:
      gpl_builder.set_blending_alpha();
      break;
    }

    gpl_builder.set_depth_test_enabled(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
    gpl_builder.set_color_attachment_format(ctx->draw_image.image_format);
    gpl_builder.set_depth_format(ctx->depth_image.image_format);

    if (const auto pipeline{gpl_builder.build(ctx->device)}; !pipeline) {
      log_error("Unable to build graphics pipeline");
      return Err{pipeline.error()};
    } else {
      database->pipeline = *pipeline;
    }
  }

  log_info("Sprite database {} destroying shaders modules", dbh);
  destroy_shader_module(ctx, *fragment_shader);
  destroy_shader_module(ctx, *vertex_shader);

  // Creation done
  log_info("Created sprite database, handle {}", dbh);
  return database;
}

void surge::renderer::vk::atom::sprite_database::destroy(Context ctx, SpriteDatabase database) {
  log_info("Destroying sprite database, handle {}", static_cast<void *>(database));

  // Destroy Vulkan objects
  vkDeviceWaitIdle(ctx->device);

  vkDestroyPipeline(ctx->device, database->pipeline, get_alloc_callbacks());

  vkDestroyPipelineLayout(ctx->device, database->pipeline_layout, get_alloc_callbacks());

  vkDestroyDescriptorSetLayout(ctx->device, database->img_desc_layout, get_alloc_callbacks());

  for (const auto &sampler : database->img_samplers) {
    vkDestroySampler(ctx->device, sampler, get_alloc_callbacks());
  }
  database->img_samplers.~vector();

  vkDestroyFence(ctx->device, database->data_transfer_fence, get_alloc_callbacks());

  vkDestroyFence(ctx->device, database->image_transfer_fence, get_alloc_callbacks());

  vkFreeCommandBuffers(ctx->device, database->cmd_pool, 1, &(database->cmd_buff));
  vkDestroyCommandPool(ctx->device, database->cmd_pool, get_alloc_callbacks());

  destroy_buffer(ctx, database->gpu_data_buffer);
  destroy_buffer(ctx, database->cpu_data_buffer);

  // Destroy descriptor infos
  database->img_desc_infos.~vector();

  // Destroy GPU images
  for (const auto &img : database->img_dst_images) {
    destroy_image(ctx, img);
  }
  database->img_dst_images.~vector();

  // Destroy CPU images
  for (const auto &buffer : database->img_src_buffers) {
    destroy_buffer(ctx, buffer);
  }
  database->img_src_buffers.~vector();

  // Destroy handle
  allocators::mimalloc::free(database);
}

auto surge::renderer::vk::atom::sprite_database::upload_images(Context ctx, SpriteDatabase database,
                                                               std::span<const char *> paths)
    -> Result<void> {
  const auto dbh{static_cast<void *>(database)};

  const auto num_paths{paths.size()};

  // Condition 1: Is it possible to upload num_paths images to the device?
  const bool cond_1{(database->img_dst_images.size() + num_paths) <= database->max_sprites};

  // Condition 2: Is it possible to add num_path images to the source buffers?
  const bool cond_2{(database->img_src_buffers.size() + num_paths) <= database->max_sprites};

  if (!(cond_1 && cond_2)) {
    log_error("Unable to upload more images to sprite database {}. Database full.", dbh);
    return Err{vk_atom_sprite_database_full};
  }

  // 1. Load image data into host buffers and add it to the src buffer vector

  // TODO: Read data from actual image
  {
    for (const auto &path : paths) {
      // TODO: Get image data
      const auto image_data{gen_random_img<8>()};

      // Create source buffer
      auto img_src_buffer{create_buffer(ctx, image_data.total_image_size,
                                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                        VMA_MEMORY_USAGE_CPU_TO_GPU)};

      if (!img_src_buffer) {
        log_error("Sprite database {} unable to create source buffer for texture {}", dbh, path);
        return Err{img_src_buffer.error()};
      }

      // Transfer image data to source buffer
      memcpy(img_src_buffer->info.pMappedData, image_data.colors.data(),
             image_data.total_image_size);

      // Save the buffer to source vector
      database->img_src_buffers.push_back(*img_src_buffer);
    }
  }

  // 2. Allocate GPU image data to receive the uploads
  {
    for (usize i = 0; i < database->img_src_buffers.size(); i++) {
      // Create destination buffer
      // TODO: Need to find a way to get the extent from the image data
      VkExtent3D tmp_extent{8, 8, 1};
      auto img_dest_image{create_image(ctx, tmp_extent, VK_FORMAT_R8G8B8A8_UNORM,
                                       VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
                                           | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                       false)};

      if (!img_dest_image) {
        log_error("Sprite database {} unable to create GPU memory for texture {}", dbh, paths[i]);
        return Err{img_dest_image.error()};
      }

      database->img_dst_images.push_back(*img_dest_image);
    }
  }

  // 3. Submit transfer command
  {
    // Reset fences
    auto result{vkResetFences(ctx->device, 1, &database->image_transfer_fence)};

    if (result != VK_SUCCESS) {
      log_error("Sprite database {} unable to reset image transfer fence", dbh);
      return Err{vk_atom_sprite_database_fence_reset};
    }

    result = vkResetCommandBuffer(database->cmd_buff, 0);

    if (result != VK_SUCCESS) {
      log_error("Sprite database {} unable to reset command buffer for texture transfer operation",
                dbh);
      return Err{vk_atom_sprite_database_command_buffer_reset};
    }

    auto cmd{database->cmd_buff};
    auto cmd_buff_beg_info{command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)};

    result = vkBeginCommandBuffer(cmd, &cmd_buff_beg_info);

    if (result != VK_SUCCESS) {
      log_error("Sprite database {} unable to begin texture transfer command recording", dbh);
      return Err{vk_atom_sprite_database_command_buffer_begin};
    }

    // Begin transfer commands
    for (usize i = 0; i < database->img_src_buffers.size(); i++) {
      auto &src_buffer{database->img_src_buffers[i]};
      auto &dst_image{database->img_dst_images[i]};

      transition_image(cmd, dst_image.image, VK_IMAGE_LAYOUT_UNDEFINED,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

      VkBufferImageCopy copyRegion{};
      copyRegion.bufferOffset = 0;
      copyRegion.bufferRowLength = 0;
      copyRegion.bufferImageHeight = 0;

      copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      copyRegion.imageSubresource.mipLevel = 0;
      copyRegion.imageSubresource.baseArrayLayer = 0;
      copyRegion.imageSubresource.layerCount = 1;
      copyRegion.imageExtent = dst_image.image_extent;

      vkCmdCopyBufferToImage(cmd, src_buffer.buffer, dst_image.image,
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

      transition_image(cmd, dst_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
    // End transfer commands

    result = vkEndCommandBuffer(cmd);

    if (result != VK_SUCCESS) {
      log_error("Sprite database {} unable to end texture transfer command recording", dbh);
      return Err{vk_atom_sprite_database_command_buffer_end};
    }

    // Submit
    auto buffer_submit_info{command_buffer_submit_info(cmd)};
    auto sub_info{submit_info(&buffer_submit_info, nullptr, nullptr)};

    result = vkQueueSubmit2(ctx->q_handles.transfer, 1, &sub_info, database->image_transfer_fence);

    if (result != VK_SUCCESS) {
      log_error("Sprite database {} unable to submit texture transfer commands", dbh);
      return Err{vk_atom_sprite_database_command_buffer_submit};
    }
  }

  // 4. Wait submission
  {
    log_info("Sprite database {} is uploading textures to GPU", dbh);

    const auto result{
        vkWaitForFences(ctx->device, 1, &database->image_transfer_fence, true, 10000000000)};

    if (result != VK_SUCCESS) {
      log_error("Sprite database {} unable to upload textures", dbh);
      return Err{vk_atom_sprite_database_command_buffer_sync};
    }
  }

  // 5. Reset source buffers
  {
    for (auto &buffer : database->img_src_buffers) {
      destroy_buffer(ctx, buffer);
    }
    database->img_src_buffers.clear();
  }

  return {};
}

void surge::renderer::vk::atom::sprite_database::update_draw_data(SpriteDatabase database,
                                                                  const UpdateInfo &update_info) {
  using std::memcpy;

  if ((database->cpu_data_buffer_size + 1) <= database->max_sprites) {

    // Texture
    {
      VkDescriptorImageInfo image_info{
          .sampler = database->img_samplers[update_info.texture_id],
          .imageView = database->img_dst_images[update_info.texture_id].image_view,
          .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

      database->img_desc_infos.push_back(image_info);
    }

    // Geometry
    {
      const auto model_matrix{
          make_model_matrix(update_info.position, update_info.scale, update_info.z)};

      const SpriteData sprite_data{.model_matrix = model_matrix,
                                   .color_multiplier = update_info.color_multiplier};

      const auto offset{database->cpu_data_buffer_size * sprite_data_size};

      memcpy(static_cast<char *>(database->cpu_data_buffer.info.pMappedData) + offset, &sprite_data,
             sprite_data_size);

      database->cpu_data_buffer_size += 1;
    }

  } else {
    log_warn("Unable to update sprite database {}: Database full", static_cast<void *>(database));
  }
}

void surge::renderer::vk::atom::sprite_database::update_draw_data(
    SpriteDatabase database, std::span<const UpdateInfo> update_infos) {
  for (const auto &info : update_infos) {
    update_draw_data(database, info);
  }
}

auto surge::renderer::vk::atom::sprite_database::synchronize_draw_data(Context ctx,
                                                                       SpriteDatabase database)
    -> Result<void> {
  // Reset fences
  {
    const auto result{vkResetFences(ctx->device, 1, &database->data_transfer_fence)};

    if (result != VK_SUCCESS) {
      log_error("Unable to reset sprite database {} buffer transfer fence",
                static_cast<void *>(database));
      return Err{vk_atom_sprite_database_fence_reset};
    }
  }

  // Reset command buffer
  {
    const auto result{vkResetCommandBuffer(database->cmd_buff, 0)};

    if (result != VK_SUCCESS) {
      log_error("Unable to reset sprite database {} command buffer", static_cast<void *>(database));
      return Err{vk_atom_sprite_database_command_buffer_reset};
    }
  }

  // Record transfer commands
  {
    auto cmd{database->cmd_buff};
    auto cmd_buff_beg_info{command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)};

    auto result{vkBeginCommandBuffer(cmd, &cmd_buff_beg_info)};

    if (result != VK_SUCCESS) {
      log_error("Unable to begin sprite database {} transfer command recording",
                static_cast<void *>(database));
      return Err{vk_atom_sprite_database_command_buffer_begin};
    }

    // Begin transfer commands
    VkBufferCopy staging_buffer_copy{};
    staging_buffer_copy.dstOffset = 0;
    staging_buffer_copy.srcOffset = 0;
    staging_buffer_copy.size = database->cpu_data_buffer_size * sprite_data_size;

    vkCmdCopyBuffer(cmd, database->cpu_data_buffer.buffer, database->gpu_data_buffer.buffer, 1,
                    &staging_buffer_copy);
    // End transfer commands

    result = vkEndCommandBuffer(cmd);

    if (result != VK_SUCCESS) {
      log_error("Unable to end sprite database {} transfer command reccording",
                static_cast<void *>(database));
      return Err{vk_atom_sprite_database_command_buffer_end};
    }

    // Submit transfer commands
    auto buffer_submit_info{command_buffer_submit_info(cmd)};
    const auto sub_info{submit_info(&buffer_submit_info, nullptr, nullptr)};

    result = vkQueueSubmit2(ctx->q_handles.transfer, 1, &sub_info, database->data_transfer_fence);

    if (result != VK_SUCCESS) {
      log_error("Unable to submit sprite database {} transfer commands",
                static_cast<void *>(database));
      return Err{vk_atom_sprite_database_command_buffer_submit};
    }
  }

  // Wait for transfer
  {
    const auto result{
        vkWaitForFences(ctx->device, 1, &database->data_transfer_fence, true, 10000000000)};

    if (result != VK_SUCCESS) {
      log_error("Unable to synchronize sprite database {} transfer commands",
                static_cast<void *>(database));
      return Err{vk_atom_sprite_database_command_buffer_sync};
    }
  }

  // Update sizes
  {
    database->gpu_data_buffer_size = database->cpu_data_buffer_size;
    database->cpu_data_buffer_size = 0;
  }

  return {};
}

void surge::renderer::vk::atom::sprite_database::draw(Context ctx, SpriteDatabase database,
                                                      const glm::mat4 &projection_matrix,
                                                      const glm::mat4 &view_matrix) {
  const auto &cmd_buff{ctx->frm_data.command_buffers[ctx->frm_data.frame_idx]};

  auto color_attachment{rendering_attachment_info(ctx->draw_image.image_view, nullptr,
                                                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)};
  auto depth_attachment{
      depth_attachment_info(ctx->depth_image.image_view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)};

  const VkExtent2D draw_extent{.width = ctx->draw_image.image_extent.width,
                               .height = ctx->draw_image.image_extent.height};

  const auto ri{rendering_info(draw_extent, &color_attachment, &depth_attachment)};

  const auto num_sprites{static_cast<u32>(database->img_desc_infos.size())};

  // Begin
  vkCmdBeginRendering(cmd_buff, &ri);

  vkCmdBindPipeline(cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, database->pipeline);

  VkWriteDescriptorSet write{};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.pNext = nullptr;
  write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write.dstSet = database->img_desc_set;
  write.dstBinding = 0;
  write.descriptorCount = num_sprites;
  write.dstArrayElement = 0;
  write.pImageInfo = database->img_desc_infos.data();

  vkUpdateDescriptorSets(ctx->device, 1, &write, 0, nullptr);
  vkCmdBindDescriptorSets(cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, database->pipeline_layout, 0,
                          1, &database->img_desc_set, 0, nullptr);

  PushConstants push_constants{.projection = projection_matrix,
                               .view = view_matrix,
                               .sprite_data_buffer_address = database->gpu_data_buffer_address};

  vkCmdPushConstants(cmd_buff, database->pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                     push_consants_size, &push_constants);

  VkViewport viewport = {};
  viewport.x = 0;
  viewport.y = 0;
  viewport.width = static_cast<float>(draw_extent.width);
  viewport.height = static_cast<float>(draw_extent.height);
  viewport.minDepth = 0.f;
  viewport.maxDepth = 1.f;

  vkCmdSetViewport(cmd_buff, 0, 1, &viewport);

  VkRect2D scissor = {};
  scissor.offset.x = 0;
  scissor.offset.y = 0;
  scissor.extent.width = draw_extent.width;
  scissor.extent.height = draw_extent.height;

  vkCmdSetScissor(cmd_buff, 0, 1, &scissor);

  vkCmdDraw(cmd_buff, 6, num_sprites, 0, 0);

  vkCmdEndRendering(cmd_buff);

  database->img_desc_infos.clear();
}