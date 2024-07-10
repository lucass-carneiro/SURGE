#ifndef SURGE_CORE_RENDERER_VK_HPP
#define SURGE_CORE_RENDERER_VK_HPP

#include "config.hpp"
#include "error_types.hpp"

// clang-format off
#include "vk_bootstrap/VkBootstrap.hpp"
#include <vk_mem_alloc.h>
// clang-format on

#include <optional>

namespace surge::renderer::vk {

struct swapchain_data {
  vkb::Swapchain swapchain{};
  vector<VkImage> imgs{};
  vector<VkImageView> imgs_views{};
};

struct frame_cmd_data {
  VkCommandPool pool{};
  VkCommandBuffer buffer{};
};

struct frame_sync_data {
  VkSemaphore swpc_semaphore{nullptr};
  VkSemaphore render_semaphore{nullptr};
  VkFence render_fence{nullptr};
};

struct frame_data {
  static constexpr usize frame_overlap{2};
  usize frame_idx{0};

  u32 graphics_queue_family{0};
  VkQueue graphics_queue{nullptr};

  std::array<VkCommandPool, frame_overlap> command_pools{};
  std::array<VkCommandBuffer, frame_overlap> command_buffers{};

  std::array<VkSemaphore, frame_overlap> swpc_semaphores{};
  std::array<VkSemaphore, frame_overlap> render_semaphores{};
  std::array<VkFence, frame_overlap> render_fences{};

  inline void advance_idx() noexcept {
    frame_idx = frame_idx + 1 < frame_overlap ? frame_idx + 1 : 0;
  }
};

struct allocated_image {
  VkImage image{nullptr};
  VkImageView image_view{nullptr};
  VmaAllocation allocation{nullptr};
  VkExtent3D image_extent{};
  VkFormat image_format{};
};

struct context {
  vkb::Instance instance{};
  VkSurfaceKHR surface{};

  vkb::PhysicalDevice phys_device{};
  vkb::Device device{};

  swapchain_data swpc_data{};
  allocated_image draw_image{};
  VkExtent2D draw_extent{};

  frame_data frm_data{};

  VmaAllocator allocator{};
};

auto command_pool_create_info(u32 queue_family_idx, VkCommandPoolCreateFlags flags
                                                    = 0) noexcept -> VkCommandPoolCreateInfo;
auto command_buffer_alloc_info(VkCommandPool pool,
                               u32 count = 1) noexcept -> VkCommandBufferAllocateInfo;
auto command_buffer_begin_info(VkCommandBufferUsageFlags flags = 0);

auto fence_create_info(VkFenceCreateFlags flags = 0) noexcept -> VkFenceCreateInfo;
auto semaphore_create_info(VkSemaphoreCreateFlags flags = 0) noexcept -> VkSemaphoreCreateInfo;

auto image_subresource_range(VkImageAspectFlags aspect_mask) noexcept -> VkImageSubresourceRange;

auto semaphore_submit_info(VkPipelineStageFlags2 stage_mask,
                           VkSemaphore semaphore) noexcept -> VkSemaphoreSubmitInfo;
auto command_buffer_submit_info(VkCommandBuffer cmd) noexcept -> VkCommandBufferSubmitInfo;
auto submit_info(VkCommandBufferSubmitInfo *cmd, VkSemaphoreSubmitInfo *signal_sem_info,
                 VkSemaphoreSubmitInfo *wai_sem_info) noexcept -> VkSubmitInfo2;

auto image_create_info(VkFormat format, VkImageUsageFlags usage_flags,
                       VkExtent3D extent) noexcept -> VkImageCreateInfo;
auto imageview_create_info(VkFormat format, VkImage image,
                           VkImageAspectFlags aspect_flags) noexcept -> VkImageViewCreateInfo;

void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout curr_layout,
                      VkImageLayout new_layout) noexcept;

void image_blit(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D src_size,
                VkExtent2D dst_size) noexcept;

auto clear(context &ctx, const config::clear_color &ccl) noexcept -> std::optional<error>;

auto create_swapchain(const config::renderer_attrs &r_attrs, context &ctx, u32 width,
                      u32 height) noexcept -> tl::expected<swapchain_data, error>;
void destroy_swapchain(context &ctx, swapchain_data &swpc) noexcept;

auto init(const config::renderer_attrs &r_attrs, const config::window_resolution &w_res,
          const config::window_attrs &w_attrs) noexcept -> tl::expected<context, error>;
void terminate(context &ctx);

} // namespace surge::renderer::vk

#endif // SURGE_CORE_RENDERER_VK_HPP