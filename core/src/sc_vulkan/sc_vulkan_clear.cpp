#include "sc_vulkan/sc_vulkan.hpp"
#include "sc_vulkan/sc_vulkan_images.hpp"

void surge::renderer::vk::clear_swpc(Context ctx, const config::ClearColor &w_ccl) {
  auto &cmd_buff{ctx->frm_data.command_buffers[ctx->frm_data.frame_idx]};

  // Clear color
  VkClearColorValue clear_value{{w_ccl.r, w_ccl.g, w_ccl.b, w_ccl.a}};

  VkImageSubresourceRange clear_range{image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT)};
  vkCmdClearColorImage(cmd_buff, ctx->draw_image.image, VK_IMAGE_LAYOUT_GENERAL, &clear_value, 1,
                       &clear_range);
}