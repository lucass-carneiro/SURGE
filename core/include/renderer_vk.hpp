#ifndef SURGE_CORE_RENDERER_VK_HPP
#define SURGE_CORE_RENDERER_VK_HPP

#include "config.hpp"
#include "error_types.hpp"
#include "options.hpp"

// clang-format off
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
// clang-format on

#include <optional>
#include <array>

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
  VkDevice log_dev{};

  VkSurfaceKHR surface{};
  queue_handles q_handles{};
  swapchain_data swpc_data{};

  frame_data frm_data{};

  VmaAllocator allocator{};

  allocated_image draw_image{};
};

} // namespace surge::renderer::vk

namespace surge::renderer::vk::infos {

auto dbg_msg_create_info() noexcept -> VkDebugUtilsMessengerCreateInfoEXT;

auto command_pool_create_info(u32 queue_family_idx, VkCommandPoolCreateFlags flags
                                                    = 0) noexcept -> VkCommandPoolCreateInfo;
auto command_buffer_alloc_info(VkCommandPool pool,
                               u32 count = 1) noexcept -> VkCommandBufferAllocateInfo;
auto command_buffer_begin_info(VkCommandBufferUsageFlags flags
                               = 0) noexcept -> VkCommandBufferBeginInfo;

auto fence_create_info(VkFenceCreateFlags flags = 0) noexcept -> VkFenceCreateInfo;
auto semaphore_create_info(VkSemaphoreCreateFlags flags = 0) noexcept -> VkSemaphoreCreateInfo;

auto command_buffer_submit_info(VkCommandBuffer cmd) noexcept -> VkCommandBufferSubmitInfo;
auto semaphore_submit_info(VkPipelineStageFlags2 stage_mask,
                           VkSemaphore semaphore) noexcept -> VkSemaphoreSubmitInfo;
auto submit_info(VkCommandBufferSubmitInfo *cmd, VkSemaphoreSubmitInfo *signam_sem_info,
                 VkSemaphoreSubmitInfo *wai_sem_info) noexcept -> VkSubmitInfo2;

auto imageview_create_info(VkFormat format, VkImage image,
                           VkImageAspectFlags aspect_flags) noexcept -> VkImageViewCreateInfo;

} // namespace surge::renderer::vk::infos

namespace surge::renderer::vk::init_helpers {

auto get_required_extensions() noexcept -> tl::expected<vector<const char *>, error>;

#ifdef SURGE_USE_VK_VALIDATION_LAYERS
auto get_required_validation_layers() noexcept -> tl::expected<vector<const char *>, error>;

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

auto create_frame_data(VkDevice device,
                       u32 graphics_queue_idx) noexcept -> tl::expected<frame_data, error>;
void destroy_frame_data(context &ctx) noexcept;

auto create_memory_allocator(VkInstance instance, VkPhysicalDevice phys_dev,
                             VkDevice logi_dev) noexcept -> tl::expected<VmaAllocator, error>;

auto image_create_info(VkFormat format, VkImageUsageFlags usage_flags,
                       VkExtent3D extent) noexcept -> VkImageCreateInfo;

void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout curr_layout,
                      VkImageLayout new_layout) noexcept;
void image_blit(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D src_size,
                VkExtent2D dst_size) noexcept;
auto image_subresource_range(VkImageAspectFlags aspect_mask) noexcept -> VkImageSubresourceRange;

auto create_draw_img(const config::window_resolution &w_res, VkDevice logi_dev,
                     VmaAllocator allocator) noexcept -> tl::expected<allocated_image, error>;

} // namespace surge::renderer::vk::init_helpers

namespace surge::renderer::vk {

auto clear(context &ctx, const config::clear_color &w_ccl) noexcept -> std::optional<error>;

auto init(const config::renderer_attrs &r_attrs, const config::window_resolution &w_res,
          const config::window_attrs &w_attrs) noexcept -> tl::expected<context, error>;

void terminate(context &ctx) noexcept;

} // namespace surge::renderer::vk

#endif // SURGE_CORE_RENDERER_VK_HPP