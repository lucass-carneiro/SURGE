#ifndef SURGE_RENDERER_VULKAN_DEBUG_HPP
#define SURGE_RENDERER_VULKAN_DEBUG_HPP

#include "sc_error_types.hpp"
#include "sc_options.hpp"

// clang-format off
#include <vulkan/vulkan.h>
// clang-format on

#include <tl/expected.hpp>

namespace surge::renderer::vk {

#ifdef SURGE_USE_VK_VALIDATION_LAYERS
auto dbg_msg_create_info() -> VkDebugUtilsMessengerCreateInfoEXT;

auto create_dbg_msg(VkInstance instance) -> tl::expected<VkDebugUtilsMessengerEXT, error>;

auto create_dbg_msg(VkInstance instance, VkDebugUtilsMessengerCreateInfoEXT create_info)
    -> tl::expected<VkDebugUtilsMessengerEXT, error>;

auto destroy_dbg_msg(VkInstance instance,
                     VkDebugUtilsMessengerEXT dbg_msg) -> tl::expected<void, error>;
#endif

} // namespace surge::renderer::vk

#endif // SURGE_RENDERER_VULKAN_DEBUG_HPP