#ifndef SURGE_CORE_RENDERER_VULKAN_HPP
#define SURGE_CORE_RENDERER_VULKAN_HPP

#include "sc_config.hpp"
#include "sc_container_types.hpp"
#include "sc_integer_types.hpp"
#include "sc_options.hpp"
#include "sc_window.hpp"

#include <tl/expected.hpp>

// clang-format off
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
// clang-format on

#include <array>
#include <optional>

namespace surge::renderer::vk {

struct queue_family_indices {
  std::optional<u32> graphics_family{};
  std::optional<u32> present_family{};
  std::optional<u32> transfer_family{};
  std::optional<u32> compute_family{};
};

struct queue_handles {
  u32 graphics_idx{};
  u32 transfer_idx{};
  u32 compute_idx{};

  VkQueue graphics{};
  VkQueue transfer{};
  VkQueue compute{};
};

struct swapchain_data {
  VkSwapchainKHR swapchain{};
  VkExtent2D extent{};
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
  VkInstance instance{};

#ifdef SURGE_USE_VK_VALIDATION_LAYERS
  VkDebugUtilsMessengerEXT dbg_msg{};
#endif

  VkPhysicalDevice phys_dev{};
  VkDevice device{};

  VkSurfaceKHR surface{};
  queue_handles q_handles{};
  swapchain_data swpc_data{};

  frame_data frm_data{};

  VmaAllocator allocator{};

  allocated_image draw_image{};
};

auto initialize(window::window_t w, const config::renderer_attrs &r_attrs,
                const config::window_resolution &w_res,
                const config::window_attrs &w_attrs) noexcept -> tl::expected<context, error>;

void terminate(context &ctx) noexcept;

auto request_img(context &ctx) noexcept -> tl::expected<std::tuple<VkImage, u32>, error>;
auto present(context &ctx, u32 &swpc_img_idx) noexcept -> std::optional<error>;

auto cmd_begin(context &ctx) noexcept -> std::optional<error>;
auto cmd_end(context &ctx) noexcept -> std::optional<error>;
auto cmd_submit(context &ctx) noexcept -> std::optional<error>;

void clear(context &ctx, VkImage swpc_image, const config::clear_color &w_ccl) noexcept;

} // namespace surge::renderer::vk

#endif // SURGE_CORE_RENDERER_VULKAN_HPP