#ifndef SURGE_CORE_RENDERER_VK_HPP
#define SURGE_CORE_RENDERER_VK_HPP

#include "config.hpp"
#include "error_types.hpp"
#include "options.hpp"

// clang-format off
#include <vulkan/vulkan.h>
// clang-format on

#include <optional>

namespace surge::renderer::vk {

struct queue_family_indices {
  std::optional<u32> graphics_family{};
  std::optional<u32> present_family{};
  std::optional<u32> transfer_family{};
  std::optional<u32> compute_family{};
};

struct queue_handles {
  VkQueue graphics{};
  VkQueue transfer{};
  VkQueue compute{};
};

struct swapchain_data {
  VkSwapchainKHR swapchain{};
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

struct context {
  VkInstance instance{};
#ifdef SURGE_USE_VK_VALIDATION_LAYERS
  VkDebugUtilsMessengerEXT dbg_msg{};
#endif
  VkPhysicalDevice phys_dev{};
  VkDevice log_dev{};
  VkSurfaceKHR surface{};
  queue_handles q_handles{};
  swapchain_data swpc_data{};
  frame_data frm_data{};
};

auto clear(context &ctx, const config::clear_color &ccl) noexcept -> std::optional<error>;

namespace init_helpers {

auto get_required_extensions() noexcept -> tl::expected<vector<const char *>, error>;

#ifdef SURGE_USE_VK_VALIDATION_LAYERS
auto get_required_validation_layers() noexcept -> tl::expected<vector<const char *>, error>;

auto dbg_msg_create_info() noexcept -> VkDebugUtilsMessengerCreateInfoEXT;
auto create_dbg_msg(VkInstance instance) noexcept -> tl::expected<VkDebugUtilsMessengerEXT, error>;
auto destroy_dbg_msg(VkInstance instance,
                     VkDebugUtilsMessengerEXT dbg_msg) -> tl::expected<void, error>;
#endif

auto create_instance() noexcept -> tl::expected<VkInstance, error>;

auto find_queue_families(VkPhysicalDevice phys_dev) noexcept -> queue_family_indices;

auto get_required_device_extensions(VkPhysicalDevice phys_dev) noexcept
    -> tl::expected<vector<const char *>, error>;
auto is_device_suitable(VkPhysicalDevice phys_dev) noexcept -> bool;
auto select_physical_device(VkInstance instance) noexcept -> tl::expected<VkPhysicalDevice, error>;
auto create_logical_device(VkPhysicalDevice phys_dev) noexcept -> tl::expected<VkDevice, error>;

auto create_window_surface(VkInstance instance) noexcept -> tl::expected<VkSurfaceKHR, error>;

auto get_queue_handles(VkPhysicalDevice phys_dev, VkDevice log_dev,
                       VkSurfaceKHR surface) noexcept -> tl::expected<queue_handles, error>;

auto create_swapchain(VkPhysicalDevice phys_dev, VkDevice log_dev, VkSurfaceKHR surface,
                      const config::renderer_attrs &r_attrs, u32 width,
                      u32 height) noexcept -> tl::expected<swapchain_data, error>;

auto command_pool_create_info(u32 queue_family_idx,
                              VkCommandPoolCreateFlags flags) noexcept -> VkCommandPoolCreateInfo;
auto command_buffer_alloc_info(VkCommandPool pool,
                               u32 count) noexcept -> VkCommandBufferAllocateInfo;

auto create_frame_data() noexcept -> tl::expected<frame_data, error>;

} // namespace init_helpers

auto init(const config::renderer_attrs &r_attrs, const config::window_resolution &w_res,
          const config::window_attrs &w_attrs) noexcept -> tl::expected<context, error>;

void terminate(context &ctx) noexcept;

} // namespace surge::renderer::vk

#endif // SURGE_CORE_RENDERER_VK_HPP