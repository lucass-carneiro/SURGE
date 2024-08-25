#ifndef SURGE_VK_ATOM_SHADER_HPP
#define SURGE_VK_ATOM_SHADER_HPP

#include "error_types.hpp"

#include <array>
#include <tl/expected.hpp>
#include <vulkan/vulkan.h>

namespace surge::vk_atom::shader {

auto load_shader_module(VkDevice device,
                        const char *path) noexcept -> tl::expected<VkShaderModule, error>;

void destroy_shader_module(VkDevice device, VkShaderModule module) noexcept;

using program_shaders = std::array<VkShaderEXT, 2>;

auto create_shader_object(VkInstance instance, VkDevice device, const char *vertex_shader_path,
                          const char *fragment_shader_path) noexcept
    -> tl::expected<program_shaders, error>;

void destroy_shader_object(VkInstance instance, VkDevice device, program_shaders &shaders) noexcept;

} // namespace surge::vk_atom::shader

#endif // SURGE_VK_ATOM_SHADER_HPP