#ifndef SURGE_CORE_RENDERER_VULKAN_INIT_HPP
#define SURGE_CORE_RENDERER_VULKAN_INIT_HPP

#include "sc_config.hpp"
#include "sc_container_types.hpp"
#include "sc_error_types.hpp"
#include "sc_glfw_includes.hpp"
#include "sc_options.hpp"
#include "sc_vulkan_types.hpp"

#include <tl/expected.hpp>

namespace surge::renderer::vk {

auto get_api_version() noexcept -> tl::expected<u32, error>;

auto get_required_extensions() noexcept -> tl::expected<vector<const char *>, error>;

#ifdef SURGE_USE_VK_VALIDATION_LAYERS
auto get_required_validation_layers() noexcept -> tl::expected<vector<const char *>, error>;
#endif

#ifdef SURGE_USE_VK_VALIDATION_LAYERS
auto build_instance(const vector<const char *> &required_extensions,
                    const vector<const char *> &required_validation_layers,
                    const VkDebugUtilsMessengerCreateInfoEXT &dbg_msg_ci) noexcept
    -> tl::expected<VkInstance, error>;
#else
auto build_instance(const vector<const char *> &required_extensions) noexcept
    -> tl::expected<VkInstance, error>;
#endif

auto select_physical_device(VkInstance instance) noexcept -> tl::expected<VkPhysicalDevice, error>;

auto is_device_suitable(VkPhysicalDevice phys_dev) noexcept -> bool;

auto find_queue_families(VkPhysicalDevice phys_dev) noexcept -> queue_family_indices;

auto get_required_device_extensions(VkPhysicalDevice phys_dev) noexcept
    -> tl::expected<vector<const char *>, error>;

auto create_logical_device(VkPhysicalDevice phys_dev) noexcept -> tl::expected<VkDevice, error>;

auto create_window_surface(window::window_t w, VkInstance instance) noexcept -> tl::expected<VkSurfaceKHR, error>;

auto get_queue_handles(VkPhysicalDevice phys_dev, VkDevice log_dev, VkSurfaceKHR surface) noexcept
    -> tl::expected<queue_handles, error>;

auto create_swapchain(VkPhysicalDevice phys_dev, VkDevice log_dev, VkSurfaceKHR surface,
                      const config::renderer_attrs &r_attrs, u32 width, u32 height) noexcept
    -> tl::expected<swapchain_data, error>;

auto create_frame_data(VkDevice device, u32 graphics_queue_idx) noexcept
    -> tl::expected<frame_data, error>;

void destroy_frame_data(VkDevice device, frame_data &frm_data) noexcept;

} // namespace surge::renderer::vk

#endif // SURGE_CORE_RENDERER_VULKAN_INIT_HPP