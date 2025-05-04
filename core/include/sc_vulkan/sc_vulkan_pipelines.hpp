#ifndef SURGE_RENDERER_VULKAN_PIPELINES_HPP
#define SURGE_RENDERER_VULKAN_PIPELINES_HPP

#include "sc_container_types.hpp"
#include "sc_error_types.hpp"
#include "sc_vulkan/sc_vulkan.hpp"

#include <vulkan/vulkan.h>

namespace surge::renderer::vk {

auto load_shader_module(Context ctx, const char *spirv_path) -> Result<VkShaderModule>;
void destroy_shader_module(Context ctx, VkShaderModule shader_module);

} // namespace surge::renderer::vk

#endif // SURGE_RENDERER_VULKAN_PIPELINES_HPP