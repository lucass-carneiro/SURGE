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

auto get_api_version() -> tl::expected<u32, error>;

auto get_required_extensions() -> tl::expected<vector<const char *>, error>;

#ifdef SURGE_USE_VK_VALIDATION_LAYERS
auto get_required_validation_layers() -> tl::expected<vector<const char *>, error>;
#endif

#ifdef SURGE_USE_VK_VALIDATION_LAYERS
auto build_instance(const vector<const char *> &required_extensions,
                    const vector<const char *> &required_validation_layers,
                    const VkDebugUtilsMessengerCreateInfoEXT &dbg_msg_ci)
    -> tl::expected<VkInstance, error>;
#else
auto build_instance(const vector<const char *> &required_extensions)
    -> tl::expected<VkInstance, error>;
#endif

auto select_physical_device(VkInstance instance) -> tl::expected<VkPhysicalDevice, error>;

auto is_device_suitable(VkPhysicalDevice phys_dev) -> bool;

auto find_queue_families(VkPhysicalDevice phys_dev) -> queue_family_indices;

auto get_required_device_extensions(VkPhysicalDevice phys_dev)
    -> tl::expected<vector<const char *>, error>;

auto create_logical_device(VkPhysicalDevice phys_dev) -> tl::expected<VkDevice, error>;

auto create_window_surface(window::window_t w,
                           VkInstance instance) -> tl::expected<VkSurfaceKHR, error>;

auto get_queue_handles(VkPhysicalDevice phys_dev, VkDevice log_dev,
                       VkSurfaceKHR surface) -> tl::expected<queue_handles, error>;

auto create_swapchain(VkPhysicalDevice phys_dev, VkDevice log_dev, VkSurfaceKHR surface,
                      const config::renderer_attrs &r_attrs, u32 width,
                      u32 height) -> tl::expected<swapchain_data, error>;

auto create_frame_data(VkDevice device, u32 graphics_queue_idx) -> tl::expected<frame_data, error>;

void destroy_frame_data(VkDevice device, frame_data &frm_data);

} // namespace surge::renderer::vk

#endif // SURGE_CORE_RENDERER_VULKAN_INIT_HPP