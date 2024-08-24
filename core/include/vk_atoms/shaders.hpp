#ifndef SURGE_VK_ATOM_SHADER_HPP
#define SURGE_VK_ATOM_SHADER_HPP

#include "error_types.hpp"

#include <tl/expected.hpp>
#include <vulkan/vulkan.h>

namespace surge::vk_atom::shader {

auto load_shader_module(VkDevice device,
                        const char *path) noexcept -> tl::expected<VkShaderModule, error>;

void destroy_shader_module(VkDevice device, VkShaderModule module) noexcept;

} // namespace surge::vk_atom::shader

#endif // SURGE_VK_ATOM_SHADER_HPP