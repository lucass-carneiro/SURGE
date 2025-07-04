// https://gist.github.com/DethRaid/0171f3cfcce51950ee4ef96c64f59617

#include "sc_vulkan/atoms/sprite_database.hpp"

#include "sc_allocators.hpp"
#include "sc_logging.hpp"
#include "sc_vulkan/sc_vulkan_command.hpp"
#include "sc_vulkan/sc_vulkan_images.hpp"
#include "sc_vulkan/sc_vulkan_malloc.hpp"
#include "sc_vulkan/sc_vulkan_pipelines.hpp"
#include "sc_vulkan/sc_vulkan_sync.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <vulkan/vk_enum_string_helper.h>

// GPU sprite data
struct SpriteData {
  alignas(16) glm::mat4 model_matrix{1.0};      // Model matrix
  alignas(16) glm::vec4 color_multiplier{1.0f}; // Color modifier
};

// Sprite database push constants for shaders
struct PushConstants {
  glm::mat4 projection{1.0};                     // Global Projection matrix
  glm::mat4 view{1.0};                           // Global view matrix
  VkDeviceAddress sprite_data_buffer_address{0}; // GPU Address to sprite data buffer
};

struct surge::renderer::vk::atom::sprite_database::SpriteDatabaseT {
  VkPipelineLayout pipeline_layout{VK_NULL_HANDLE}; // Vulkan pipeline layout for the database.
  VkPipeline pipeline{VK_NULL_HANDLE};              // Vulkan pipeline for the database.

  VkFence buffer_fence{VK_NULL_HANDLE};  // Sync. sprite data buffer transfer operations.
  VkFence texture_fence{VK_NULL_HANDLE}; // Sync. sprite data buffer transfer operations.

  VkCommandBuffer cmd_buff{VK_NULL_HANDLE}; // Database operations cmd. buffer.
  VkCommandPool cmd_pool{VK_NULL_HANDLE};   // Database operations cmd. pool.

  usize max_sprites{0};      // How many sprites we can add.
  usize curr_num_sprites{0}; // How many sprites we added pushed so far.
  u32 num_sub_sprites{0};    // How many sprites we've actually sent to the gpu last cycle.

  Buffer sprite_data_staging_buffer{};              // CPU staging buffer for sprite data.
  Buffer sprite_data_gpu_buffer{};                  // GPU buffer with sprite data.
  VkDeviceAddress sprite_data_gpu_buffer_address{}; // Address of GPU buffer with sprite data.

  AllocatedImage default_image{};                                    // Default missing texture
  containers::mimalloc::Vector<AllocatedImage> sprite_textures{};    // GPU sprite textures
  containers::mimalloc::Vector<VkDescriptorImageInfo> image_infos{}; // Texture descriptor infos.
  containers::mimalloc::Vector<VkSampler> sprite_texture_samplers{}; // GPU sprite samplers.

  VkDescriptorSetLayout sprite_texture_set_layout{VK_NULL_HANDLE}; // Sprite texture desc. layout
  VkDescriptorSet sprite_texture_set{VK_NULL_HANDLE};              // Sprite texture desc. set
};

static auto
transfer_img_data_immediate(surge::renderer::vk::atom::sprite_database::SpriteDatabase database,
                            surge::renderer::vk::Context ctx, void *data, VkExtent3D size,
                            VkFormat format, VkImageUsageFlags usage, bool mipmapped)
    -> surge::Result<surge::renderer::vk::AllocatedImage> {
  using namespace surge;
  using namespace surge::renderer::vk;
  using std::memcpy;

  const auto dbh{static_cast<void *>(database)};

  // Create staging buffer
  log_info("Sprite database {} creating staging texture buffer", dbh);

  const auto data_size{size.depth * size.width * size.height * 4};
  auto upload_buffer{
      create_buffer(ctx, data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU)};

  if (!upload_buffer) {
    log_error("Sprite database {} unable to create staging texture buffer", dbh);
    return Err{upload_buffer.error()};
  }

  memcpy(upload_buffer->info.pMappedData, data, data_size);

  // Create GPU memory for the texture
  log_info("Sprite database {} creating GPU memory for texture data", dbh);

  auto new_image{create_image(
      ctx, size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
      mipmapped)};

  if (!new_image) {
    log_error("Sprite database {} unable to create GPU memory for texture data", dbh);
    return Err{new_image.error()};
  }

  // Reset fences
  auto result{vkResetFences(ctx->device, 1, &database->texture_fence)};

  if (result != VK_SUCCESS) {
    log_error("Sprite database {} unable to reset  texture transfer fence: {}", dbh,
              string_VkResult(result));
    return Err{vk_atom_sprite_database_fence_reset};
  }

  result = vkResetCommandBuffer(database->cmd_buff, 0);

  if (result != VK_SUCCESS) {
    log_error("Sprite database {} unable command buffer for texture transfer operation: {}", dbh,
              string_VkResult(result));
    return Err{vk_atom_sprite_database_command_buffer_reset};
  }

  auto cmd{database->cmd_buff};
  auto cmd_buff_beg_info{command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)};

  result = vkBeginCommandBuffer(cmd, &cmd_buff_beg_info);

  if (result != VK_SUCCESS) {
    log_error("Sprite database {} unable to begin texture transfer command recording: {}", dbh,
              string_VkResult(result));
    return Err{vk_atom_sprite_database_command_buffer_begin};
  }

  // Begin transfer commands
  transition_image(cmd, new_image->image, VK_IMAGE_LAYOUT_UNDEFINED,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  VkBufferImageCopy copyRegion{};
  copyRegion.bufferOffset = 0;
  copyRegion.bufferRowLength = 0;
  copyRegion.bufferImageHeight = 0;

  copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  copyRegion.imageSubresource.mipLevel = 0;
  copyRegion.imageSubresource.baseArrayLayer = 0;
  copyRegion.imageSubresource.layerCount = 1;
  copyRegion.imageExtent = size;

  vkCmdCopyBufferToImage(cmd, upload_buffer->buffer, new_image->image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

  transition_image(cmd, new_image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  // End transfer commands

  result = vkEndCommandBuffer(cmd);

  if (result != VK_SUCCESS) {
    log_error("Sprite database {} unable to end texture transfer command recording: {}", dbh,
              string_VkResult(result));
    return Err{vk_atom_sprite_database_command_buffer_end};
  }

  auto buffer_submit_info{command_buffer_submit_info(cmd)};
  auto sub_info{submit_info(&buffer_submit_info, nullptr, nullptr)};

  result = vkQueueSubmit2(ctx->q_handles.transfer, 1, &sub_info, database->texture_fence);

  if (result != VK_SUCCESS) {
    log_error("Sprite database {} unable to submit texture transfer commands: {}", dbh,
              string_VkResult(result));
    return Err{vk_atom_sprite_database_command_buffer_submit};
  }

  result = vkWaitForFences(ctx->device, 1, &database->texture_fence, true, 10000000000);

  if (result != VK_SUCCESS) {
    log_error("Sprite database {} unable to synchronize transfer commands: {}", dbh,
              string_VkResult(result));
    return Err{vk_atom_sprite_database_command_buffer_sync};
  }

  destroy_buffer(ctx, *upload_buffer);

  return new_image;
}

auto surge::renderer::vk::atom::sprite_database::create(const SpriteDatabaseCreateInfo &ci,
                                                        GrowableDescriptorAllocator &desc_alloc)
    -> Result<SpriteDatabase> {
  log_info("Creating sprite database");

  auto &ctx{ci.ctx};

  // Allocate object handle
  auto database{static_cast<SpriteDatabase>(allocators::mimalloc::malloc(sizeof(SpriteDatabaseT)))};

  if (database == nullptr) {
    log_error("Unable to allocate sprite database");
    return Err{vk_atom_sprite_database_init};
  }

  new (database) SpriteDatabaseT();

  // Database handle as void*, used for printing
  const auto dbh{static_cast<void *>(database)};

  // Sprite data buffers sizes
  database->max_sprites = ci.max_sprites;
  const auto buffer_size{database->max_sprites * sizeof(SpriteData)};

  // Reserve vectors
  database->sprite_textures.reserve(database->max_sprites);
  database->image_infos.reserve(database->max_sprites);
  database->sprite_texture_samplers.reserve(database->max_sprites);

  // Cmd pool
  log_info("Creating sprite database {} command pool", dbh);

  auto cmd_pool_create_info{command_pool_create_info(
      ctx->q_handles.transfer_idx, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)};
  auto result{vkCreateCommandPool(ctx->device, &cmd_pool_create_info, get_alloc_callbacks(),
                                  &database->cmd_pool)};

  if (result != VK_SUCCESS) {
    log_error("Unable to allocate sprite database {} command pool: {}", dbh,
              string_VkResult(result));
    return Err{vk_cmd_pool_creation};
  }

  // Cmd buffer
  log_info("Creating sprite database {} command buffer", dbh);

  const auto cmd_buffer_info{command_buffer_alloc_info(database->cmd_pool, 1)};
  result = vkAllocateCommandBuffers(ctx->device, &cmd_buffer_info, &database->cmd_buff);

  if (result != VK_SUCCESS) {
    log_error("Unable to allocate sprite database {} command buffer: {}", dbh,
              string_VkResult(result));
    return Err{vk_cmd_buffer_creation};
  }

  // Fences
  log_info("Creating sprite database {} fences", dbh);

  auto fence_ci{fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT)};
  result = vkCreateFence(ctx->device, &fence_ci, get_alloc_callbacks(), &database->buffer_fence);

  if (result != VK_SUCCESS) {
    log_error("Unable to create sprite database {} buffer transfer fence: {}", dbh,
              string_VkResult(result));
    return Err{vk_fence_creation};
  }

  fence_ci = fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
  result = vkCreateFence(ctx->device, &fence_ci, get_alloc_callbacks(), &database->texture_fence);

  if (result != VK_SUCCESS) {
    log_error("Unable to create sprite database {} texture transfer fence: {}", dbh,
              string_VkResult(result));
    return Err{vk_fence_creation};
  }

  // Sprite data CPU staging buffer
  log_info("Creating sprite database {} data CPU staging buffer", dbh);

  VkBufferCreateInfo staging_ci{};
  staging_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  staging_ci.pNext = nullptr;
  staging_ci.size = buffer_size;
  staging_ci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

  VmaAllocationCreateInfo staging_ai{};
  staging_ai.usage = VMA_MEMORY_USAGE_AUTO;
  staging_ai.flags
      = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

  result = vmaCreateBuffer(
      ctx->allocator, &staging_ci, &staging_ai, &database->sprite_data_staging_buffer.buffer,
      &database->sprite_data_staging_buffer.allocation, &database->sprite_data_staging_buffer.info);

  if (result != VK_SUCCESS) {
    log_error("Unable to create sprite database {} data CPU staging buffer: {}", dbh,
              string_VkResult(result));
    return Err{vk_buffer_allocation};
  }

  // Sprite data GPU buffers
  log_info("Creating sprite database {} GPU data buffer", dbh);

  const auto gpu_buffer{create_buffer(ctx, buffer_size,
                                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
                                          | VK_BUFFER_USAGE_TRANSFER_DST_BIT
                                          | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                      VMA_MEMORY_USAGE_GPU_ONLY)};

  if (!gpu_buffer) {
    log_error("Unable to create sprite database {} GPU data buffer", dbh);
    return Err{gpu_buffer.error()};
  }

  database->sprite_data_gpu_buffer = *gpu_buffer;

  // Get data buffer address
  log_info("Getting sprite database {} GPU data buffer address", dbh);

  VkBufferDeviceAddressInfo gpu_buffer_address_info{};
  gpu_buffer_address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
  gpu_buffer_address_info.buffer = database->sprite_data_gpu_buffer.buffer;

  database->sprite_data_gpu_buffer_address
      = vkGetBufferDeviceAddress(ctx->device, &gpu_buffer_address_info);

  // Default checkerboard image
  const auto black{glm::packUnorm4x8(glm::vec4(0, 0, 0, 1))};
  const auto white{glm::packUnorm4x8(glm::vec4(1, 1, 1, 1))};

  std::array<u32, 16 * 16> pixels{};
  for (usize x = 0; x < 16; x++) {
    for (usize y = 0; y < 16; y++) {
      pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? white : black;
    }
  }

  // TODO: Don't immediate submit. Maybe submit during sync?
  const auto default_image{
      transfer_img_data_immediate(database, ctx, pixels.data(), VkExtent3D{16, 16, 1},
                                  VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false)};

  if (!default_image) {
    log_error("Sprite database {} unable to create default sprite image", dbh);
    return Err{default_image.error()};
  }

  database->default_image = *default_image;

  // Sprite textures descriptor
  // TODO: choose types of filters. Do mip maps
  log_info("Creating sprite textures sampler");
  VkSamplerCreateInfo texture_sampler{};
  texture_sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  texture_sampler.pNext = nullptr;
  texture_sampler.magFilter = VK_FILTER_NEAREST;
  texture_sampler.minFilter = VK_FILTER_NEAREST;

  for (usize i = 0; i < database->max_sprites; i++) {
    VkSampler sampler{};
    result = vkCreateSampler(ctx->device, &texture_sampler, get_alloc_callbacks(), &sampler);

    if (result != VK_SUCCESS) {
      log_error("Unable to create sprite database texture sampler: {}", string_VkResult(result));
      return Err{vk_sampler_creation};
    }

    database->sprite_texture_samplers.push_back(sampler);
  }

  log_info("Creating sprite textures descriptor set layout");
  VkDescriptorSetLayoutBinding texture_descriptor_binding{};
  texture_descriptor_binding.binding = 0;
  texture_descriptor_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  texture_descriptor_binding.descriptorCount = static_cast<u32>(database->max_sprites);
  texture_descriptor_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  texture_descriptor_binding.pImmutableSamplers = database->sprite_texture_samplers.data();

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

  result = vkCreateDescriptorSetLayout(ctx->device, &texture_descriptor_set_layout_info,
                                       get_alloc_callbacks(), &database->sprite_texture_set_layout);

  if (result != VK_SUCCESS) {
    log_error("Unable to create sprite database texture descriptor set layout: {}",
              string_VkResult(result));
    return Err{vk_descriptor_set_layout_build};
  }

  log_info("Allocating sprite textures descriptor set");
  std::array<u32, 1> descriptor_counts{{static_cast<u32>(database->max_sprites)}};

  VkDescriptorSetVariableDescriptorCountAllocateInfo variable_descriptor_count_info{};
  variable_descriptor_count_info.sType
      = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
  variable_descriptor_count_info.pNext = nullptr;
  variable_descriptor_count_info.descriptorSetCount = static_cast<u32>(descriptor_counts.size());
  variable_descriptor_count_info.pDescriptorCounts = descriptor_counts.data();

  const auto sprite_texture_set{desc_alloc.allocate(ctx, database->sprite_texture_set_layout,
                                                    &variable_descriptor_count_info)};

  if (!sprite_texture_set) {
    log_error("Unable to allocate sprite database texture descriptor set");
    return Err{sprite_texture_set.error()};
  }

  database->sprite_texture_set = *sprite_texture_set;

  // Shaders
  log_info("Loading sprite database shaders");

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
  log_info("Creating sprite database pipeline layout");

  VkPushConstantRange push_constant_range{};
  push_constant_range.offset = 0;
  push_constant_range.size = sizeof(PushConstants);
  push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkPipelineLayoutCreateInfo pipeline_layout_ci{};
  pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_ci.pNext = nullptr;

  pipeline_layout_ci.pPushConstantRanges = &push_constant_range;
  pipeline_layout_ci.pushConstantRangeCount = 1;

  pipeline_layout_ci.pSetLayouts = &database->sprite_texture_set_layout;
  pipeline_layout_ci.setLayoutCount = 1;

  result = vkCreatePipelineLayout(ctx->device, &pipeline_layout_ci, get_alloc_callbacks(),
                                  &database->pipeline_layout);

  if (result != VK_SUCCESS) {
    log_error("Unable create compute pipeline layout: {}", string_VkResult(result));
    return Err{vk_pipeline_layout_create};
  }

  // Pipeline
  log_info("Building sprite database pipeline");

  GraphicsPipelineBuilder gpl_builder{};
  gpl_builder.set_layout(database->pipeline_layout);
  gpl_builder.set_shaders(*vertex_shader, *fragment_shader);
  gpl_builder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  gpl_builder.set_polygon_mode(VK_POLYGON_MODE_FILL);
  gpl_builder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
  gpl_builder.set_multisampling_none();

  switch (ci.blending_mode) {
  case SpriteDatabaseBlendingMode::none:
    gpl_builder.set_blending_none();
    break;
  case SpriteDatabaseBlendingMode::additive:
    gpl_builder.set_blending_additive();
    break;
  case SpriteDatabaseBlendingMode::alpha:
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

  log_info("Destroying shader modules");
  destroy_shader_module(ctx, *fragment_shader);
  destroy_shader_module(ctx, *vertex_shader);

  log_info("Successfully created sprite database, handle {}", static_cast<void *>(database));
  return database;
}

void surge::renderer::vk::atom::sprite_database::destroy(SpriteDatabase database, Context ctx) {
  log_info("Destroying sprite database, handle {}", static_cast<void *>(database));

  // Destroy Vulkan objects
  vkDeviceWaitIdle(ctx->device);

  destroy_buffer(ctx, database->sprite_data_staging_buffer);
  destroy_buffer(ctx, database->sprite_data_gpu_buffer);

  vkDestroyDescriptorSetLayout(ctx->device, database->sprite_texture_set_layout,
                               get_alloc_callbacks());

  for (const auto &sampler : database->sprite_texture_samplers) {
    vkDestroySampler(ctx->device, sampler, get_alloc_callbacks());
  }

  destroy_image(ctx, database->default_image);

  vkDestroyFence(ctx->device, database->texture_fence, get_alloc_callbacks());
  vkDestroyFence(ctx->device, database->buffer_fence, get_alloc_callbacks());
  vkDestroyCommandPool(ctx->device, database->cmd_pool, get_alloc_callbacks());

  vkDestroyPipeline(ctx->device, database->pipeline, get_alloc_callbacks());
  vkDestroyPipelineLayout(ctx->device, database->pipeline_layout, get_alloc_callbacks());

  // Destroy handle
  allocators::mimalloc::free(database);
}

auto surge::renderer::vk::atom::sprite_database::add(SpriteDatabase database,
                                                     SpriteDatabaseAddInfo add_info)
    -> Result<void> {
  if (database->curr_num_sprites < database->max_sprites) {
    // TODO: Load and send actual image to GPU. Here we will simply send the default texture
    database->sprite_textures.push_back(database->default_image);

    VkDescriptorImageInfo image_info{
        .sampler = database->sprite_texture_samplers[database->sprite_textures.size()],
        .imageView = database->default_image.image_view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

    database->image_infos.push_back(image_info);

    // Add data to staging buffer
    const auto model_matrix{make_model_matrix(add_info.position, add_info.scale, add_info.z)};

    const SpriteData sprite_data{.model_matrix = model_matrix,
                                 .color_multiplier = add_info.color_multiplier};

    constexpr auto sprite_data_size{sizeof(SpriteData)};
    const auto offset{database->curr_num_sprites * sizeof(SpriteData)};

    memcpy(static_cast<char *>(database->sprite_data_staging_buffer.info.pMappedData) + offset,
           &sprite_data, sprite_data_size);

    database->curr_num_sprites += 1;
  } else {
    log_warn("Sprite database {} unable to add new sprite. Database is full",
             static_cast<void *>(database));
  }

  return {};
}

void surge::renderer::vk::atom::sprite_database::push(SpriteDatabase database,
                                                      const SpriteDatabasePushInfo &push_info) {
  using std::memcpy;

  if (database->curr_num_sprites < database->max_sprites) {
    const auto model_matrix{make_model_matrix(push_info.position, push_info.scale, push_info.z)};

    const SpriteData sprite_data{.model_matrix = model_matrix,
                                 .color_multiplier = push_info.color_multiplier};

    constexpr auto sprite_data_size{sizeof(SpriteData)};
    const auto offset{database->curr_num_sprites * sizeof(SpriteData)};

    memcpy(static_cast<char *>(database->sprite_data_staging_buffer.info.pMappedData) + offset,
           &sprite_data, sprite_data_size);

    database->curr_num_sprites += 1;
  } else {
    log_warn("Unable to allocate new sprite to database handle {}. Database is full.",
             static_cast<void *>(database));
  }
}

auto surge::renderer::vk::atom::sprite_database::sync(SpriteDatabase database, Context ctx)
    -> Result<void> {
  auto result{vkResetFences(ctx->device, 1, &database->buffer_fence)};

  if (result != VK_SUCCESS) {
    log_error("Unable to reset sprite database {} buffer transfer fence: {}",
              static_cast<void *>(database), string_VkResult(result));
    return Err{vk_atom_sprite_database_fence_reset};
  }

  result = vkResetCommandBuffer(database->cmd_buff, 0);

  if (result != VK_SUCCESS) {
    log_error("Unable to reset sprite database {} command buffer: {}",
              static_cast<void *>(database), string_VkResult(result));
    return Err{vk_atom_sprite_database_command_buffer_reset};
  }

  auto cmd{database->cmd_buff};
  auto cmd_buff_beg_info{command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)};

  result = vkBeginCommandBuffer(cmd, &cmd_buff_beg_info);

  if (result != VK_SUCCESS) {
    log_error("Unable to begin sprite database {} transfer command recording: {}",
              static_cast<void *>(database), string_VkResult(result));
    return Err{vk_atom_sprite_database_command_buffer_begin};
  }

  // Begin transfer commands
  VkBufferCopy staging_buffer_copy{};
  staging_buffer_copy.dstOffset = 0;
  staging_buffer_copy.srcOffset = 0;
  staging_buffer_copy.size = database->curr_num_sprites * sizeof(SpriteData);

  vkCmdCopyBuffer(cmd, database->sprite_data_staging_buffer.buffer,
                  database->sprite_data_gpu_buffer.buffer, 1, &staging_buffer_copy);
  // End transfer commands

  result = vkEndCommandBuffer(cmd);

  if (result != VK_SUCCESS) {
    log_error("Unable to end sprite database {} transfer command reccording: {}",
              static_cast<void *>(database), string_VkResult(result));
    return Err{vk_atom_sprite_database_command_buffer_end};
  }

  auto buffer_submit_info{command_buffer_submit_info(cmd)};
  auto sub_info{submit_info(&buffer_submit_info, nullptr, nullptr)};

  result = vkQueueSubmit2(ctx->q_handles.transfer, 1, &sub_info, database->buffer_fence);

  if (result != VK_SUCCESS) {
    log_error("Unable to submit sprite database {} transfer commands: {}",
              static_cast<void *>(database), string_VkResult(result));
    return Err{vk_atom_sprite_database_command_buffer_submit};
  }

  result = vkWaitForFences(ctx->device, 1, &database->buffer_fence, true, 10000000000);

  if (result != VK_SUCCESS) {
    log_error("Unable to synchronize sprite database {} transfer commands: {}",
              static_cast<void *>(database), string_VkResult(result));
    return Err{vk_atom_sprite_database_command_buffer_sync};
  }

  database->num_sub_sprites = static_cast<u32>(database->curr_num_sprites);
  database->curr_num_sprites = 0;

  return {};
}

void surge::renderer::vk::atom::sprite_database::draw(SpriteDatabase database, Context ctx,
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

  vkCmdBeginRendering(cmd_buff, &ri);

  vkCmdBindPipeline(cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, database->pipeline);

  VkWriteDescriptorSet write{};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.pNext = nullptr;
  write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write.dstSet = database->sprite_texture_set;
  write.dstBinding = 0;
  write.descriptorCount = static_cast<u32>(database->image_infos.size());
  write.dstArrayElement = 0;
  write.pImageInfo = database->image_infos.data();

  vkUpdateDescriptorSets(ctx->device, 1, &write, 0, nullptr);
  vkCmdBindDescriptorSets(cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, database->pipeline_layout, 0,
                          1, &database->sprite_texture_set, 0, nullptr);

  PushConstants push_constants{.projection = projection_matrix,
                               .view = view_matrix,
                               .sprite_data_buffer_address
                               = database->sprite_data_gpu_buffer_address};

  vkCmdPushConstants(cmd_buff, database->pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                     sizeof(PushConstants), &push_constants);

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

  vkCmdDraw(cmd_buff, 6, database->num_sub_sprites, 0, 0);

  vkCmdEndRendering(cmd_buff);
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