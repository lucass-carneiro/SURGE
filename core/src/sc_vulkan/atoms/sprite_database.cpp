#include "sc_vulkan/atoms/sprite_database.hpp"

#include "sc_allocators.hpp"
#include "sc_logging.hpp"
#include "sc_vulkan/sc_vulkan_malloc.hpp"
#include "sc_vulkan/sc_vulkan_pipelines.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <vulkan/vk_enum_string_helper.h>

struct surge::renderer::vk::atom::sprite_database::SpriteDatabaseT {
  VkPipelineLayout pipeline_layout{VK_NULL_HANDLE};
  VkPipeline pipeline{VK_NULL_HANDLE};
};

auto surge::renderer::vk::atom::sprite_database::create(Context ctx) -> Result<SpriteDatabase> {
  log_info("Creating sprite database");

  // Allocate object handle
  auto database{static_cast<SpriteDatabase>(allocators::mimalloc::malloc(sizeof(SpriteDatabaseT)))};

  if (database == nullptr) {
    log_error("Unable to allocate sprite database");
    return Err{vk_atom_sprite_database_init};
  }

  new (database) SpriteDatabaseT();

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

  VkPushConstantRange world_matrices{};
  world_matrices.offset = 0;
  world_matrices.size = sizeof(WorldMatrices);
  world_matrices.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkPipelineLayoutCreateInfo pipeline_layout_ci{};
  pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_ci.pNext = nullptr;

  pipeline_layout_ci.pPushConstantRanges = &world_matrices;
  pipeline_layout_ci.pushConstantRangeCount = 1;

  auto result{vkCreatePipelineLayout(ctx->device, &pipeline_layout_ci, get_alloc_callbacks(),
                                     &database->pipeline_layout)};

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
  vkDestroyPipeline(ctx->device, database->pipeline, get_alloc_callbacks());
  vkDestroyPipelineLayout(ctx->device, database->pipeline_layout, get_alloc_callbacks());

  // Destroy handle
  allocators::mimalloc::free(database);
}

void surge::renderer::vk::atom::sprite_database::draw(SpriteDatabase database, Context ctx,
                                                      const WorldMatrices &world_matrices) {
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

  vkCmdPushConstants(cmd_buff, database->pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                     sizeof(WorldMatrices), &world_matrices);

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

  vkCmdDraw(cmd_buff, 6, 1, 0, 0);

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