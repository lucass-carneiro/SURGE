#include "sc_vulkan/atoms/sprite_database.hpp"

#include "sc_allocators.hpp"
#include "sc_logging.hpp"
#include "sc_vulkan/sc_vulkan_command.hpp"
#include "sc_vulkan/sc_vulkan_malloc.hpp"
#include "sc_vulkan/sc_vulkan_pipelines.hpp"
#include "sc_vulkan/sc_vulkan_sync.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <vulkan/vk_enum_string_helper.h>

struct SpriteData {
  glm::mat4 model_matrix{1.0};
};

struct PushConstants {
  glm::mat4 projection{1.0};
  glm::mat4 view{1.0};
  VkDeviceAddress sprite_data_buffer_address{0};
};

struct surge::renderer::vk::atom::sprite_database::SpriteDatabaseT {
  VkPipelineLayout pipeline_layout{VK_NULL_HANDLE};
  VkPipeline pipeline{VK_NULL_HANDLE};

  VkFence buffer_fence{VK_NULL_HANDLE};
  VkCommandBuffer cmd_buff{VK_NULL_HANDLE};
  VkCommandPool cmd_pool{VK_NULL_HANDLE};

  usize max_sprites{0};      // How many sprites we can push
  usize curr_num_sprites{0}; // How many sprites we have pushed so far
  u32 num_sub_sprites{0};    // How many sprites we've actually sent to the gpu last sync

  Buffer sprite_data_staging_buffer{};
  Buffer sprite_data_gpu_buffer{};
  VkDeviceAddress sprite_data_gpu_buffer_address{};
};

auto surge::renderer::vk::atom::sprite_database::create(Context ctx, usize max_sprites)
    -> Result<SpriteDatabase> {
  log_info("Creating sprite database");

  // Allocate object handle
  auto database{static_cast<SpriteDatabase>(allocators::mimalloc::malloc(sizeof(SpriteDatabaseT)))};

  if (database == nullptr) {
    log_error("Unable to allocate sprite database");
    return Err{vk_atom_sprite_database_init};
  }

  new (database) SpriteDatabaseT();

  // Cmd pool
  log_info("Creating sprite database command pool");

  auto cmd_pool_create_info{command_pool_create_info(
      ctx->q_handles.transfer_idx, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)};
  auto result{vkCreateCommandPool(ctx->device, &cmd_pool_create_info, get_alloc_callbacks(),
                                  &database->cmd_pool)};

  if (result != VK_SUCCESS) {
    log_error("Unable to allocate sprite database command pool: {}", string_VkResult(result));
    return Err{Error::vk_cmd_pool_creation};
  }

  // Cmd buffer
  log_info("Creating sprite database command buffer");

  const auto cmd_buffer_info{command_buffer_alloc_info(database->cmd_pool, 1)};
  result = vkAllocateCommandBuffers(ctx->device, &cmd_buffer_info, &database->cmd_buff);

  if (result != VK_SUCCESS) {
    log_error("Unable to allocate sprite database command buffer: {}", string_VkResult(result));
    return Err{Error::vk_cmd_buffer_creation};
  }

  // Fence fro synchronizing buffer copies
  log_info("Creating sprite database internal buffer fence");

  auto fence_ci{fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT)};
  result = vkCreateFence(ctx->device, &fence_ci, get_alloc_callbacks(), &database->buffer_fence);

  if (result != VK_SUCCESS) {
    log_error("Unable to create immediate mode fence: {}", string_VkResult(result));
    return Err{Error::vk_fence_creation};
  }

  // Sprite data buffers sizes
  database->max_sprites = max_sprites;
  const auto buffer_size{database->max_sprites * sizeof(SpriteData)};

  // Sprite data CPU  staging buffer
  log_info("Creating sprite database internal CPU staging buffer");

  VkBufferCreateInfo staging_buffer_info{};
  staging_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  staging_buffer_info.pNext = nullptr;
  staging_buffer_info.size = buffer_size;
  staging_buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

  VmaAllocationCreateInfo staging_alloc_info{};
  staging_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
  staging_alloc_info.flags
      = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

  result = vmaCreateBuffer(ctx->allocator, &staging_buffer_info, &staging_alloc_info,
                           &database->sprite_data_staging_buffer.buffer,
                           &database->sprite_data_staging_buffer.allocation,
                           &database->sprite_data_staging_buffer.info);

  if (result != VK_SUCCESS) {
    log_error("Unable to create sprite database CPU staging buffer: {}", string_VkResult(result));
    return Err{vk_buffer_allocation};
  }

  // Sprite data GPU buffers
  log_info("Creating sprite database internal GPU data buffer");

  const auto gpu_buffer{create_buffer(ctx, buffer_size,
                                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
                                          | VK_BUFFER_USAGE_TRANSFER_DST_BIT
                                          | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                      VMA_MEMORY_USAGE_GPU_ONLY)};

  if (!gpu_buffer) {
    log_error("Unable to create sprite database GPU data buffer");
    return Err{gpu_buffer.error()};
  } else {
    database->sprite_data_gpu_buffer = *gpu_buffer;

    VkBufferDeviceAddressInfo gpu_buffer_address_info{};
    gpu_buffer_address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    gpu_buffer_address_info.buffer = database->sprite_data_gpu_buffer.buffer;

    database->sprite_data_gpu_buffer_address
        = vkGetBufferDeviceAddress(ctx->device, &gpu_buffer_address_info);
  }

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
  gpl_builder.set_blending_none();
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

  vkDestroyFence(ctx->device, database->buffer_fence, get_alloc_callbacks());
  vkDestroyCommandPool(ctx->device, database->cmd_pool, get_alloc_callbacks());

  vkDestroyPipeline(ctx->device, database->pipeline, get_alloc_callbacks());
  vkDestroyPipelineLayout(ctx->device, database->pipeline_layout, get_alloc_callbacks());

  // Destroy handle
  allocators::mimalloc::free(database);
}

void surge::renderer::vk::atom::sprite_database::push(SpriteDatabase database,
                                                      const glm::vec2 &position,
                                                      const glm::vec2 &scale, float z) {
  using std::memcpy;

  if (database->curr_num_sprites < database->max_sprites) {
    const auto model_matrix{make_model_matrix(position, scale, z)};

    const SpriteData sprite_data{.model_matrix = model_matrix};

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
  VkBufferCopy staging_buffer_copy{0};
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