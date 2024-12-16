#include "sc_vulkan/sc_vulkan.hpp"
#include "sc_vulkan_images.hpp"

void surge::renderer::vk::clear(context &ctx, VkImage swpc_image,
                                const config::clear_color &w_ccl) noexcept {
  auto &cmd_buff{ctx.frm_data.command_buffers[ctx.frm_data.frame_idx]};

  // transition our main draw image into general layout so we can write into it we will overwrite it
  // all so we dont care about what was the older layout
  transition_image(cmd_buff, ctx.draw_image.image, VK_IMAGE_LAYOUT_UNDEFINED,
                   VK_IMAGE_LAYOUT_GENERAL);

  // Clear color
  VkClearColorValue clear_value{{w_ccl.r, w_ccl.g, w_ccl.b, w_ccl.a}};

  VkImageSubresourceRange clear_range{image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT)};
  vkCmdClearColorImage(cmd_buff, ctx.draw_image.image, VK_IMAGE_LAYOUT_GENERAL, &clear_value, 1,
                       &clear_range);

  // Transition the draw image and the swapchain image into their correct transfer layouts
  transition_image(cmd_buff, ctx.draw_image.image, VK_IMAGE_LAYOUT_GENERAL,
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
  transition_image(cmd_buff, swpc_image, VK_IMAGE_LAYOUT_UNDEFINED,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  // Copy draw image to swapchain image
  VkExtent2D draw_image_ext_2d{ctx.draw_image.image_extent.width,
                               ctx.draw_image.image_extent.height};
  image_blit(cmd_buff, ctx.draw_image.image, swpc_image, draw_image_ext_2d, ctx.swpc_data.extent);

  // Make the swapchain image presentable
  transition_image(cmd_buff, swpc_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
}