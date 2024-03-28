#ifndef SURGE_RENDERER_HPP
#define SURGE_RENDERER_HPP

#include "VkBootstrap.hpp"
#include "config.hpp"
#include "container_types.hpp"
#include "error_types.hpp"
#include "vk_mem_alloc.h"

#include <tl/expected.hpp>

namespace surge::renderer {

struct frame_data {
  VkCommandPool pool{};
  VkCommandBuffer buffer{};

  VkSemaphore sc_sem{};
  VkSemaphore render_sem{};
  VkFence render_fence{};
};

struct context {
  vkb::Instance instance{};
  vkb::Device device{};

  VkSurfaceKHR surface{};

  vkb::Swapchain swapchain{};
  vector<VkImage> images{};
  vector<VkImageView> image_views{};

  u32 graphics_queue_family{};
  VkQueue graphics_queue{};

  std::array<frame_data, 3> ofd{};

  VmaAllocator allocator{};

  auto get_current_frame() noexcept -> frame_data &;
};

auto init(const config::window_attrs &wattrs, GLFWwindow *window) noexcept
    -> tl::expected<context, error>;
void terminate(context &ctx) noexcept;

auto cmd_buff_beg_info(VkCommandBufferUsageFlags flags = 0) noexcept -> VkCommandBufferBeginInfo;

void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout src,
                      VkImageLayout dest) noexcept;

auto image_subresource_range(VkImageAspectFlags aspectMask) -> VkImageSubresourceRange;

auto semaphore_submit_info(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore) noexcept
    -> VkSemaphoreSubmitInfo;

auto command_buffer_submit_info(VkCommandBuffer cmd) noexcept -> VkCommandBufferSubmitInfo;

auto submit_info(VkCommandBufferSubmitInfo *cmd, VkSemaphoreSubmitInfo *signalSemaphoreInfo,
                 VkSemaphoreSubmitInfo *waitSemaphoreInfo) noexcept -> VkSubmitInfo2;

} // namespace surge::renderer

#endif // SURGE_RENDERER_HPP