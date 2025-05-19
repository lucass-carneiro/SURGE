#include "sc_vulkan/sc_vulkan_pipelines.hpp"

#include "sc_files.hpp"
#include "sc_logging.hpp"
#include "sc_vulkan/sc_vulkan_malloc.hpp"

#include <array>
#include <vulkan/vk_enum_string_helper.h>

auto surge::renderer::vk::load_shader_module(Context ctx,
                                             const char *spirv_path) -> Result<VkShaderModule> {

  log_info("Loading {} as SPIRV shader module", spirv_path);

  const auto file_data{files::as_bytes(spirv_path, false)};
  if (!file_data) {
    log_error("Unable to read SPIRV file {}", spirv_path);
    return Err{file_data.error()};
  }

  // Vulkan wants the spirv data to be a u32 buffer
  const auto spirv_data{reinterpret_cast<const u32 *>(file_data->data())};

  // create a new shader module, using the buffer we loaded
  VkShaderModuleCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.pNext = nullptr;
  create_info.codeSize = file_data->size();
  create_info.pCode = spirv_data;

  VkShaderModule shader_module{};
  const auto result{
      vkCreateShaderModule(ctx->device, &create_info, get_alloc_callbacks(), &shader_module)};

  if (result != VK_SUCCESS) {
    log_error("Unable create shader module from SPIRV file {}: {}", spirv_path,
              string_VkResult(result));
    return Err{Error::vk_shader_module_create};
  } else {
    log_info("Loaded {} as SPIRV shader module, handle {}", spirv_path,
             static_cast<void *>(shader_module));
    return shader_module;
  }
}

void surge::renderer::vk::destroy_shader_module(Context ctx, VkShaderModule shader_module) {
  log_info("Destroying SPIRV shader module handle {}", static_cast<void *>(shader_module));
  vkDestroyShaderModule(ctx->device, shader_module, get_alloc_callbacks());
}

auto surge::renderer::vk::rendering_attachment_info(
    VkImageView view, VkClearValue *clear, VkImageLayout layout) -> VkRenderingAttachmentInfo {
  VkRenderingAttachmentInfo rai{};
  rai.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  rai.pNext = nullptr;

  rai.imageView = view;
  rai.imageLayout = layout;
  rai.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
  rai.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

  if (clear) {
    rai.clearValue = *clear;
  }

  return rai;
}

auto surge::renderer::vk::depth_attachment_info(VkImageView view,
                                                VkImageLayout layout) -> VkRenderingAttachmentInfo {
  VkRenderingAttachmentInfo dai{};
  dai.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  dai.pNext = nullptr;

  dai.imageView = view;
  dai.imageLayout = layout;
  dai.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  dai.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  dai.clearValue.depthStencil.depth = 0.0f;

  return dai;
}

auto surge::renderer::vk::rendering_info(
    VkExtent2D extent, VkRenderingAttachmentInfo *color_attachment,
    VkRenderingAttachmentInfo *depth_attachment) -> VkRenderingInfo {
  VkRenderingInfo ri{};
  ri.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  ri.pNext = nullptr;

  ri.renderArea = VkRect2D{VkOffset2D{0, 0}, extent};
  ri.layerCount = 1;
  ri.colorAttachmentCount = 1;
  ri.pColorAttachments = color_attachment;
  ri.pDepthAttachment = depth_attachment;
  ri.pStencilAttachment = nullptr;

  return ri;
}

surge::renderer::vk::GraphicsPipelineBuilder::GraphicsPipelineBuilder() { clear(); }

void surge::renderer::vk::GraphicsPipelineBuilder::clear() {
  input_assembly_info = VkPipelineInputAssemblyStateCreateInfo{};
  input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

  rasterizer_info = VkPipelineRasterizationStateCreateInfo{};
  rasterizer_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

  multisampling_info = VkPipelineMultisampleStateCreateInfo{};
  multisampling_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

  depth_info = VkPipelineDepthStencilStateCreateInfo{};
  depth_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

  render_info = VkPipelineRenderingCreateInfo{};
  render_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;

  shader_stage_infos.clear();
}

auto surge::renderer::vk::GraphicsPipelineBuilder::build(VkDevice device) -> Result<VkPipeline> {
  // Make viewport state from our stored viewport and scissor.
  // At the moment we wont support multiple viewports or scissors
  VkPipelineViewportStateCreateInfo viewport{};
  viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport.pNext = nullptr;

  viewport.viewportCount = 1;
  viewport.scissorCount = 1;

  // Setup dummy color blending. We arent using transparent objects yet
  // the blending is just "no blend", but we do write to the color attachment
  VkPipelineColorBlendStateCreateInfo blending = {};
  blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  blending.pNext = nullptr;

  blending.logicOpEnable = VK_FALSE;
  blending.logicOp = VK_LOGIC_OP_COPY;
  blending.attachmentCount = 1;
  blending.pAttachments = &blend_attachment;

  // Completely clear VertexInputStateCreateInfo, as we have no need for it becase we use vertex
  // pulling
  VkPipelineVertexInputStateCreateInfo vertex_input_info{};
  vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  // Dynamic state info
  const std::array<VkDynamicState, 2> state{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dynamic_info{};
  dynamic_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_info.pDynamicStates = state.data();
  dynamic_info.dynamicStateCount = 2;

  // Build the actual pipeline
  VkGraphicsPipelineCreateInfo pipeline_info{};
  pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

  // Connect the renderInfo to the pNext extension mechanism
  pipeline_info.pNext = &render_info;

  pipeline_info.stageCount = static_cast<u32>(shader_stage_infos.size());
  pipeline_info.pStages = shader_stage_infos.data();
  pipeline_info.pVertexInputState = &vertex_input_info;
  pipeline_info.pInputAssemblyState = &input_assembly_info;
  pipeline_info.pViewportState = &viewport;
  pipeline_info.pRasterizationState = &rasterizer_info;
  pipeline_info.pMultisampleState = &multisampling_info;
  pipeline_info.pColorBlendState = &blending;
  pipeline_info.pDepthStencilState = &depth_info;
  pipeline_info.layout = pipeline_layout;
  pipeline_info.pDynamicState = &dynamic_info;

  VkPipeline pipeline{};
  const auto result{vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info,
                                              get_alloc_callbacks(), &pipeline)};

  if (result != VK_SUCCESS) {
    log_error("Unable create graphics pipeline: {}", string_VkResult(result));
    return Err{Error::vk_graphics_pipeline_create};
  }

  return pipeline;
}

void surge::renderer::vk::GraphicsPipelineBuilder::set_layout(VkPipelineLayout layout) {
  pipeline_layout = layout;
}

void surge::renderer::vk::GraphicsPipelineBuilder::set_shaders(VkShaderModule vertex_shader,
                                                               VkShaderModule fragment_sader) {
  shader_stage_infos.clear();

  // Vertex Shader
  VkPipelineShaderStageCreateInfo vs_info{};
  vs_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vs_info.pNext = nullptr;
  vs_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vs_info.module = vertex_shader;
  vs_info.pName = "main";

  shader_stage_infos.push_back(vs_info);

  // Fragment Shader
  VkPipelineShaderStageCreateInfo fs_info{};
  fs_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fs_info.pNext = nullptr;
  fs_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fs_info.module = fragment_sader;
  fs_info.pName = "main";

  shader_stage_infos.push_back(fs_info);
}

void surge::renderer::vk::GraphicsPipelineBuilder::set_input_topology(
    VkPrimitiveTopology topology) {
  input_assembly_info.topology = topology;
  input_assembly_info.primitiveRestartEnable = VK_FALSE;
}

void surge::renderer::vk::GraphicsPipelineBuilder::set_polygon_mode(VkPolygonMode mode) {
  rasterizer_info.polygonMode = mode;
  rasterizer_info.lineWidth = 1.0f;
}

void surge::renderer::vk::GraphicsPipelineBuilder::set_cull_mode(VkCullModeFlags cull_mode,
                                                                 VkFrontFace front_face) {
  rasterizer_info.cullMode = cull_mode;
  rasterizer_info.frontFace = front_face;
}

void surge::renderer::vk::GraphicsPipelineBuilder::set_multisampling_none() {
  multisampling_info.sampleShadingEnable = VK_FALSE;

  // Multisampling defaulted to no multisampling (1 sample per pixel)
  multisampling_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling_info.minSampleShading = 1.0f;
  multisampling_info.pSampleMask = nullptr;

  // No alpha to coverage either
  multisampling_info.alphaToCoverageEnable = VK_FALSE;
  multisampling_info.alphaToOneEnable = VK_FALSE;
}

void surge::renderer::vk::GraphicsPipelineBuilder::set_blending_none() {
  // Default write mask
  blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                    | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  // No blending
  blend_attachment.blendEnable = VK_FALSE;
}

void surge::renderer::vk::GraphicsPipelineBuilder::set_color_attachment_format(VkFormat format) {
  color_attachment_format = format;

  render_info.colorAttachmentCount = 1;
  render_info.pColorAttachmentFormats = &color_attachment_format;
}

void surge::renderer::vk::GraphicsPipelineBuilder::set_depth_format(VkFormat format) {
  render_info.depthAttachmentFormat = format;
}

void surge::renderer::vk::GraphicsPipelineBuilder::set_depth_test_enabled(bool enable_write,
                                                                          VkCompareOp op) {
  depth_info.depthTestEnable = VK_TRUE;
  depth_info.depthWriteEnable = enable_write;
  depth_info.depthCompareOp = op;
  depth_info.depthBoundsTestEnable = VK_FALSE;
  depth_info.stencilTestEnable = VK_FALSE;
  depth_info.front = {};
  depth_info.back = {};
  depth_info.minDepthBounds = 0.f;
  depth_info.maxDepthBounds = 1.f;
}

void surge::renderer::vk::GraphicsPipelineBuilder::set_depth_test_disabled() {
  depth_info.depthTestEnable = VK_FALSE;
  depth_info.depthWriteEnable = VK_FALSE;
  depth_info.depthCompareOp = VK_COMPARE_OP_NEVER;
  depth_info.depthBoundsTestEnable = VK_FALSE;
  depth_info.stencilTestEnable = VK_FALSE;
  depth_info.front = {};
  depth_info.back = {};
  depth_info.minDepthBounds = 0.f;
  depth_info.maxDepthBounds = 1.f;
}