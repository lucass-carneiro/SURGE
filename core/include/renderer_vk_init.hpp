#ifndef SURGE_CORE_RENDERER_VK_INIT_HPP
#define SURGE_CORE_RENDERER_VK_INIT_HPP

#include "config.hpp"
#include "container_types.hpp"
#include "error_types.hpp"
#include "options.hpp"
#include "renderer_vk_types.hpp"

#include <tl/expected.hpp>

namespace surge::renderer::vk {

auto get_required_extensions() noexcept -> tl::expected<vector<const char *>, error>;

#ifdef SURGE_USE_VK_VALIDATION_LAYERS
auto get_required_validation_layers() noexcept -> tl::expected<vector<const char *>, error>;
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

} // namespace surge::renderer::vk

#endif