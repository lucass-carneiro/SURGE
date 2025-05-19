#ifndef SURGE_RENDERER_VULKAN_PIPELINES_HPP
#define SURGE_RENDERER_VULKAN_PIPELINES_HPP

#include "sc_container_types.hpp"
#include "sc_error_types.hpp"
#include "sc_vulkan/sc_vulkan.hpp"

#include <vulkan/vulkan.h>

namespace surge::renderer::vk {

auto load_shader_module(Context ctx, const char *spirv_path) -> Result<VkShaderModule>;
void destroy_shader_module(Context ctx, VkShaderModule shader_module);

auto rendering_attachment_info(VkImageView view, VkClearValue *clear,
                               VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    -> VkRenderingAttachmentInfo;

auto depth_attachment_info(VkImageView view,
                           VkImageLayout layout
                           = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) -> VkRenderingAttachmentInfo;

auto rendering_info(VkExtent2D extent, VkRenderingAttachmentInfo *color_attachment,
                    VkRenderingAttachmentInfo *depth_attachment) -> VkRenderingInfo;

class GraphicsPipelineBuilder {
private:
  containers::mimalloc::Vector<VkPipelineShaderStageCreateInfo> shader_stage_infos{};

  VkPipelineInputAssemblyStateCreateInfo input_assembly_info{};
  VkPipelineRasterizationStateCreateInfo rasterizer_info{};
  VkPipelineMultisampleStateCreateInfo multisampling_info{};
  VkPipelineDepthStencilStateCreateInfo depth_info{};
  VkPipelineRenderingCreateInfo render_info{};

  VkPipelineColorBlendAttachmentState blend_attachment{};
  VkFormat color_attachment_format{};
  VkPipelineLayout pipeline_layout{};

public:
  GraphicsPipelineBuilder();

  auto build(VkDevice ctx) -> Result<VkPipeline>;
  void clear();

  void set_layout(VkPipelineLayout layout);

  void set_shaders(VkShaderModule vertex_shader, VkShaderModule fragment_sader);

  void set_input_topology(VkPrimitiveTopology topology);

  void set_polygon_mode(VkPolygonMode mode);

  void set_cull_mode(VkCullModeFlags cullMode, VkFrontFace frontFace);

  void set_multisampling_none();

  void set_blending_none();

  void set_color_attachment_format(VkFormat format);

  void set_depth_format(VkFormat format);

  void set_depth_test_enabled(bool enable_write, VkCompareOp op);
  void set_depth_test_disabled();
};

} // namespace surge::renderer::vk

#endif // SURGE_RENDERER_VULKAN_PIPELINES_HPP