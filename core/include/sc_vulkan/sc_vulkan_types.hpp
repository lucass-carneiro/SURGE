#ifndef SURGE_CORE_VULKAN_TYPES_HPP
#define SURGE_CORE_VULKAN_TYPES_HPP

#include "sc_container_types.hpp"
#include "sc_integer_types.hpp"
#include "sc_options.hpp"

#include <array>
#include <optional>

// clang-format off
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
// clang-format on

namespace surge::renderer::vk {

struct QueueFamilyIndices {
  std::optional<u32> graphics_family{};
  std::optional<u32> present_family{};
  std::optional<u32> transfer_family{};
  std::optional<u32> compute_family{};
};

struct QueueHandles {
  u32 graphics_idx{};
  u32 transfer_idx{};
  u32 compute_idx{};

  VkQueue graphics{};
  VkQueue transfer{};
  VkQueue compute{};
};

struct SwapchainData {
  VkSwapchainKHR swapchain{};
  VkExtent2D extent{};
  containers::mimalloc::Vector<VkImage> imgs{};
  containers::mimalloc::Vector<VkImageView> imgs_views{};
};

struct ImmediateModeData {
  VkFence fence{};
  VkCommandBuffer cmd_buff{};
  VkCommandPool cmd_pool{};
};

struct FrameData {
  static constexpr usize frame_overlap{2};
  usize frame_idx{0};

  std::array<VkCommandPool, frame_overlap> command_pools{};
  std::array<VkCommandBuffer, frame_overlap> command_buffers{};

  std::array<VkSemaphore, frame_overlap> swpc_semaphores{};
  std::array<VkSemaphore, frame_overlap> render_semaphores{};
  std::array<VkFence, frame_overlap> render_fences{};

  void advance_idx() noexcept;
};

struct AllocatedImage {
  VkImage image{nullptr};
  VkImageView image_view{nullptr};
  VmaAllocation allocation{nullptr};
  VkExtent3D image_extent{};
  VkFormat image_format{};
};

struct SwapchainImage {
  VkImage image{};
  u32 index{};
};

struct ContextData {
  VkInstance instance{};

#ifdef SURGE_USE_VK_VALIDATION_LAYERS
  VkDebugUtilsMessengerEXT dbg_msg{};
#endif

  VkPhysicalDevice phys_dev{};
  VkDevice device{};

  VkSurfaceKHR surface{};
  QueueHandles q_handles{};
  SwapchainData swpc_data{};

  FrameData frm_data{};

  // ImmediateModeData immediate_data{};

  VmaAllocator allocator{};

  AllocatedImage draw_image{};

  SwapchainImage swpc_requested_img{};
};

using Context = ContextData *;

} // namespace surge::renderer::vk

#endif // SURGE_CORE_VULKAN_TYPES_HPP