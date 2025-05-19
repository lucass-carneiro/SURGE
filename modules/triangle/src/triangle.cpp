#include "triangle.hpp"

#include "sc_error_types.hpp"
#include "sc_logging.hpp"
#include "sc_vulkan/sc_vulkan_malloc.hpp"
#include "sc_vulkan/sc_vulkan_pipelines.hpp"

#include <vulkan/vk_enum_string_helper.h>

namespace globals {

static VkPipelineLayout pipeline_layout;
static VkPipeline pipeline;

} // namespace globals

static auto create_pipeline(surge::renderer::vk::Context vk_ctx) -> surge::Result<void> {
  using namespace surge;
  using namespace surge::renderer::vk;

  // Shaders
  log_info("Loading shaders");

  const auto vertex_shader{load_shader_module(vk_ctx, "shaders/triangle.vert.spv")};
  if (!vertex_shader) {
    log_error("Unable to load vertex shader shader");
    return Err{vertex_shader.error()};
  }

  const auto fragment_shader{load_shader_module(vk_ctx, "shaders/triangle.frag.spv")};
  if (!fragment_shader) {
    log_error("Unable to load fragment shader shader");
    return Err{fragment_shader.error()};
  }

  // Pipeline layout
  log_info("Creating pipeline layout");

  VkPipelineLayoutCreateInfo pipeline_layout_ci{};
  pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_ci.pNext = nullptr;

  auto result{vkCreatePipelineLayout(vk_ctx->device, &pipeline_layout_ci, get_alloc_callbacks(),
                                     &globals::pipeline_layout)};

  if (result != VK_SUCCESS) {
    log_error("Unable create compue pipeline layout: {}", string_VkResult(result));
    return Err{Error::vk_pipeline_layout_create};
  }

  // Pipeline
  log_info("Building pipeline");

  GraphicsPipelineBuilder gpl_builder{};
  gpl_builder.set_layout(globals::pipeline_layout);
  gpl_builder.set_shaders(*vertex_shader, *fragment_shader);
  gpl_builder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  gpl_builder.set_polygon_mode(VK_POLYGON_MODE_FILL);
  gpl_builder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
  gpl_builder.set_multisampling_none();
  gpl_builder.set_blending_none();
  gpl_builder.set_depth_test_enabled(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

  // connect the image format we will draw into, from draw image
  gpl_builder.set_color_attachment_format(vk_ctx->draw_image.image_format);
  gpl_builder.set_depth_format(vk_ctx->depth_image.image_format);

  // finally build the pipeline
  const auto pipeline{gpl_builder.build(vk_ctx->device)};
  if (!pipeline) {
    log_error("Unable to build graphics pipeline");
    return Err{pipeline.error()};
  } else {
    globals::pipeline = *pipeline;
  }

  log_info("Destroying shader modules");
  destroy_shader_module(vk_ctx, *fragment_shader);
  destroy_shader_module(vk_ctx, *vertex_shader);

  return {};
}

extern "C" SURGE_MODULE_EXPORT auto on_load(surge::module::Context mod_ctx) noexcept -> int {
  log_info("Creating graphics pipeline");
  const auto result{create_pipeline(mod_ctx->vk_ctx)};
  if (!result) {
    log_error("Unable to create graphics pipeline");
    return static_cast<int>(result.error());
  }

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto on_unload(surge::module::Context mod_ctx) noexcept -> int {
  using namespace surge;
  using namespace surge::renderer::vk;

  vkDeviceWaitIdle(mod_ctx->vk_ctx->device);

  log_info("Destroying compute pipeline");
  vkDestroyPipeline(mod_ctx->vk_ctx->device, globals::pipeline, get_alloc_callbacks());

  log_info("Destroying compute pipeline layout");
  vkDestroyPipelineLayout(mod_ctx->vk_ctx->device, globals::pipeline_layout, get_alloc_callbacks());

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto draw(surge::module::Context mod_ctx) noexcept -> int {
  using namespace surge;
  using namespace surge::renderer::vk;

  auto &cmd_buff{mod_ctx->vk_ctx->frm_data.command_buffers[mod_ctx->vk_ctx->frm_data.frame_idx]};

  // Draw triangle
  // begin a render pass  connected to our draw image
  auto color_attachment{rendering_attachment_info(mod_ctx->vk_ctx->draw_image.image_view, nullptr,
                                                  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)};
  auto depth_attachment{depth_attachment_info(mod_ctx->vk_ctx->depth_image.image_view,
                                              VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)};

  VkExtent2D draw_extent{.width = mod_ctx->vk_ctx->draw_image.image_extent.width,
                         .height = mod_ctx->vk_ctx->draw_image.image_extent.height};

  auto ri{rendering_info(draw_extent, &color_attachment, &depth_attachment)};

  vkCmdBeginRendering(cmd_buff, &ri);

  vkCmdBindPipeline(cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, globals::pipeline);

  // Set dynamic viewport and scissor
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

  // Launch a draw command to draw 3 vertices
  vkCmdDraw(cmd_buff, 3, 1, 0, 0);

  vkCmdEndRendering(cmd_buff);

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto update(surge::module::Context, double) noexcept -> int {
  return 0;
}

extern "C" SURGE_MODULE_EXPORT void keyboard_event(surge::window::Window, int, int, int,
                                                   int) noexcept {}

extern "C" SURGE_MODULE_EXPORT void mouse_button_event(surge::window::Window, int, int,
                                                       int) noexcept {}

extern "C" SURGE_MODULE_EXPORT void mouse_scroll_event(surge::window::Window, double,
                                                       double) noexcept {}
