#include "compute.hpp"

#include "sc_logging.hpp"
#include "sc_vulkan/sc_vulkan_descriptor.hpp"

// clang-format off
// TODO: These should be abstracted away
#include "sc_error_types.hpp"
#include "sc_vulkan/sc_vulkan_malloc.hpp"
#include "sc_vulkan/sc_vulkan_pipelines.hpp"
// clang-format on

#include <array>
#include <random>

struct ShaderColors {
  glm::vec4 color_top{};
  glm::vec4 color_bottom{};
};

namespace globals {

static surge::renderer::vk::DescriptorPoolAllocator desc_pool_alloc{};
static VkDescriptorSetLayout draw_img_desc_layout{};
static VkDescriptorSet draw_img_desc_set{};

// TODO: This should be abstracted away
static VkPipeline compute_pipeline{};
static VkPipelineLayout compute_pipeline_layout{};

static std::random_device random_device{};
static std::mt19937 random_gen{random_device()};
static std::uniform_real_distribution<float> random_distrib(0.0, 1.0);
static ShaderColors shader_colors{};

} // namespace globals

static void randomize_colors() {
  globals::shader_colors.color_top = glm::vec4{globals::random_distrib(globals::random_gen),
                                               globals::random_distrib(globals::random_gen),
                                               globals::random_distrib(globals::random_gen), 1.0};

  globals::shader_colors.color_bottom = glm::vec4{
      globals::random_distrib(globals::random_gen), globals::random_distrib(globals::random_gen),
      globals::random_distrib(globals::random_gen), 1.0};
}

static auto init_descriptor_data(surge::renderer::vk::Context ctx) -> surge::Result<void> {
  using namespace surge;
  using namespace surge::renderer::vk;

  // create a descriptor pool that will hold 10 sets with 1 image each
  std::array<DescriptorPoolSizeRatio, 1> sizes{
      DescriptorPoolSizeRatio{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}};

  const auto init_result{globals::desc_pool_alloc.init_pool(ctx, 10, sizes)};
  if (!init_result) {
    log_error("Unable to initialize image descriptor pool allocator");
    return Err{init_result.error()};
  }

  // Create descriptor set layout for the output image
  DescriptorLayoutBuilder builder{};
  builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
  const auto draw_img_desc_layout{builder.build(ctx, VK_SHADER_STAGE_COMPUTE_BIT)};

  if (!draw_img_desc_layout) {
    log_error("Unable to build output image descriptor layout");
    return Err{draw_img_desc_layout.error()};
  } else {
    globals::draw_img_desc_layout = *draw_img_desc_layout;
  }

  // Allocate descriptor from pool
  auto draw_img_desc_set{globals::desc_pool_alloc.allocate(ctx, globals::draw_img_desc_layout)};
  if (!draw_img_desc_set) {
    log_error("Unable to allocate descriptor set for output image");
    return Err{draw_img_desc_set.error()};
  }

  // Update descriptor with image data
  VkDescriptorImageInfo image_info{};
  image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  image_info.imageView = ctx->draw_image.image_view;

  VkWriteDescriptorSet image_desc_write{};
  image_desc_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  image_desc_write.pNext = nullptr;
  image_desc_write.dstBinding = 0;
  image_desc_write.dstSet = *draw_img_desc_set;
  image_desc_write.descriptorCount = 1;
  image_desc_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  image_desc_write.pImageInfo = &image_info;

  vkUpdateDescriptorSets(ctx->device, 1, &image_desc_write, 0, nullptr);

  globals::draw_img_desc_set = *draw_img_desc_set;

  return {};
}

// TODO: This function should be abstracted away
static auto create_compute_pipeline(surge::renderer::vk::Context ctx) -> surge::Result<void> {
  using namespace surge;
  using namespace surge::renderer::vk;

  // Create compute pipeline layout
  VkPushConstantRange push_constants{};
  push_constants.offset = 0;
  push_constants.size = sizeof(ShaderColors);
  push_constants.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

  VkPipelineLayoutCreateInfo pipeline_layout{};
  pipeline_layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout.pNext = nullptr;
  pipeline_layout.pSetLayouts = &globals::draw_img_desc_layout;
  pipeline_layout.setLayoutCount = 1;

  pipeline_layout.pPushConstantRanges = &push_constants;
  pipeline_layout.pushConstantRangeCount = 1;

  auto result{vkCreatePipelineLayout(ctx->device, &pipeline_layout, get_alloc_callbacks(),
                                     &globals::compute_pipeline_layout)};

  if (result != VK_SUCCESS) {
    log_error("Unable create compue pipeline layout:");
    return Err{Error::vk_pipeline_layout_create};
  }

  // Load shader module
  const auto compute_shader{load_shader_module(ctx, "shaders/gradient.comp.spv")};
  if (!compute_shader) {
    log_error("Unable to load compute shader");
    return Err{compute_shader.error()};
  }

  VkPipelineShaderStageCreateInfo shader_stage_info{};
  shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shader_stage_info.pNext = nullptr;
  shader_stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  shader_stage_info.module = *compute_shader;
  shader_stage_info.pName = "main";

  VkComputePipelineCreateInfo compute_pipeline_create_info{};
  compute_pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  compute_pipeline_create_info.pNext = nullptr;
  compute_pipeline_create_info.layout = globals::compute_pipeline_layout;
  compute_pipeline_create_info.stage = shader_stage_info;

  result = vkCreateComputePipelines(ctx->device, VK_NULL_HANDLE, 1, &compute_pipeline_create_info,
                                    get_alloc_callbacks(), &globals::compute_pipeline);

  if (result != VK_SUCCESS) {
    log_error("Unable create compute pipeline");
    return Err{Error::vk_compute_pipeline_create};
  }

  destroy_shader_module(ctx, *compute_shader);

  return {};
}

extern "C" SURGE_MODULE_EXPORT auto on_load(surge::module::Context mod_ctx) noexcept -> int {
  using namespace surge;
  using namespace surge::renderer::vk;

  log_info("Initializing descriptor data");
  auto result{init_descriptor_data(mod_ctx->vk_ctx)};
  if (!result) {
    log_error("Unable to initialize descriptor data");
    return static_cast<int>(result.error());
  }

  log_info("Creating compute pipeline");
  result = create_compute_pipeline(mod_ctx->vk_ctx);
  if (!result) {
    log_error("Unable to initialize compute pipeline");
    return static_cast<int>(result.error());
  }

  log_info("Randomizing initial shader colors");
  randomize_colors();

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto on_unload(surge::module::Context mod_ctx) noexcept -> int {
  using namespace surge::renderer::vk;

  // TODO: This should be abstracted away
  vkDeviceWaitIdle(mod_ctx->vk_ctx->device);

  log_info("Destroying compute pipeline");
  vkDestroyPipeline(mod_ctx->vk_ctx->device, globals::compute_pipeline, get_alloc_callbacks());

  // TODO: This should be abstracted away
  log_info("Destroying compute pipeline layout");
  vkDestroyPipelineLayout(mod_ctx->vk_ctx->device, globals::compute_pipeline_layout,
                          get_alloc_callbacks());

  log_info("Destroying descriptor set pool allocator");
  globals::desc_pool_alloc.destroy_pool(mod_ctx->vk_ctx);

  log_info("Destroying output image descriptor layout");
  destroy_descriptor_set_layout(mod_ctx->vk_ctx, globals::draw_img_desc_layout);

  return 0;
}

// TODO: All of this should be abstracted away
extern "C" SURGE_MODULE_EXPORT auto draw(surge::module::Context ctx) noexcept -> int {
  using namespace surge;
  using std::ceil;

  auto &cmd_buff{ctx->vk_ctx->frm_data.command_buffers[ctx->vk_ctx->frm_data.frame_idx]};

  // Bind the gradient drawing compute pipeline
  vkCmdBindPipeline(cmd_buff, VK_PIPELINE_BIND_POINT_COMPUTE, globals::compute_pipeline);

  // Bind the descriptor set containing the draw image for the compute pipeline
  vkCmdBindDescriptorSets(cmd_buff, VK_PIPELINE_BIND_POINT_COMPUTE,
                          globals::compute_pipeline_layout, 0, 1, &globals::draw_img_desc_set, 0,
                          nullptr);

  // Update push constants
  vkCmdPushConstants(cmd_buff, globals::compute_pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
                     sizeof(ShaderColors), &globals::shader_colors);

  // Execute the compute pipeline dispatch
  const auto dims{surge::window::get_dims(ctx->window)};
  const auto x{ceil(dims[0] / 16.0)};
  const auto y{ceil(dims[1] / 16.0)};
  vkCmdDispatch(cmd_buff, static_cast<u32>(x), static_cast<u32>(y), 1);

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto update(surge::module::Context, double) noexcept -> int {
  return 0;
}

extern "C" SURGE_MODULE_EXPORT void keyboard_event(surge::window::Window, int key, int, int action,
                                                   int) noexcept {
  if (key == GLFW_KEY_UP && action == GLFW_RELEASE) {
    randomize_colors();
  }
}

extern "C" SURGE_MODULE_EXPORT void mouse_button_event(surge::window::Window, int, int,
                                                       int) noexcept {}

extern "C" SURGE_MODULE_EXPORT void mouse_scroll_event(surge::window::Window, double,
                                                       double) noexcept {}
