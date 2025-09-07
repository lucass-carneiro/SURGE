#ifndef SURGE_CORE_RENDERER_VULKAN_INIT_HPP
#define SURGE_CORE_RENDERER_VULKAN_INIT_HPP

#include "sc_config.hpp"
#include "sc_container_types.hpp"
#include "sc_error_types.hpp"
#include "sc_options.hpp"
#include "sc_vulkan_types.hpp"
#include "sc_window.hpp"

#include <vulkan/vulkan.h>

namespace surge::renderer::vk {

auto get_supported_api_version() -> Result<u32>;

auto get_required_instance_extensions() -> Result<containers::mimalloc::Vector<const char *>>;

#ifdef SURGE_USE_VK_VALIDATION_LAYERS
auto get_required_validation_layers() -> Result<containers::mimalloc::Vector<const char *>>;
#endif

#ifdef SURGE_USE_VK_VALIDATION_LAYERS
auto build_instance(const containers::mimalloc::Vector<const char *> &required_instance_extensions,
                    const containers::mimalloc::Vector<const char *> &required_validation_layers,
                    const VkDebugUtilsMessengerCreateInfoEXT &dbg_msg_ci) -> Result<VkInstance>;
#else
auto build_instance(const containers::mimalloc::Vector<const char *> &required_instance_extensions)
    -> Result<VkInstance>;
#endif

auto get_available_physical_devices(VkInstance instance)
    -> Result<containers::mimalloc::Vector<VkPhysicalDevice>>;

auto get_available_device_features(VkPhysicalDevice phys_dev)
    -> std::tuple<VkPhysicalDeviceVulkan12Features, VkPhysicalDeviceVulkan13Features>;

auto get_available_device_extensions(VkPhysicalDevice phys_dev)
    -> Result<containers::mimalloc::Vector<VkExtensionProperties>>;

auto get_available_device_queue_families(VkPhysicalDevice phys_dev)
    -> containers::mimalloc::Vector<VkQueueFamilyProperties>;

auto get_required_device_features()
    -> std::tuple<VkPhysicalDeviceVulkan12Features, VkPhysicalDeviceVulkan13Features>;

auto get_required_device_extensions() -> containers::mimalloc::Vector<const char *>;

auto get_required_device_queue_families() -> containers::mimalloc::Vector<VkQueueFlagBits>;

auto device_has_required_features(VkPhysicalDevice phys_dev) -> bool;

auto device_has_required_extensions(VkPhysicalDevice phys_dev) -> bool;

auto device_has_required_queue_families(VkPhysicalDevice phys_dev) -> bool;

auto get_queue_family_indices(
    const containers::mimalloc::Vector<VkQueueFamilyProperties> &queue_families)
    -> QueueFamilyIndices;

auto select_physical_device(VkInstance instance) -> Result<VkPhysicalDevice>;

auto is_device_suitable(VkPhysicalDevice phys_dev) -> bool;

auto create_logical_device(VkPhysicalDevice phys_dev) -> Result<VkDevice>;

auto create_window_surface(window::Window w, VkInstance instance) -> Result<VkSurfaceKHR>;

auto get_queue_handles(VkPhysicalDevice phys_dev, VkDevice log_dev, VkSurfaceKHR surface)
    -> Result<QueueHandles>;

auto create_swapchain(VkPhysicalDevice phys_dev, VkDevice log_dev, VkSurfaceKHR surface,
                      const config::RendererAttributes &r_attrs, u32 width, u32 height,
                      VkSwapchainKHR old_swapchain = VK_NULL_HANDLE) -> Result<SwapchainData>;

auto create_frame_data(VkDevice device, u32 graphics_queue_idx) -> Result<FrameData>;

void destroy_frame_data(VkDevice device, FrameData &frm_data);

auto create_immediate_mode_data(VkDevice device, u32 graphics_queue_idx)
    -> Result<ImmediateModeData>;

void destroy_immediate_mode_data(VkDevice device, ImmediateModeData &data);

} // namespace surge::renderer::vk

#endif // SURGE_CORE_RENDERER_VULKAN_INIT_HPP