#ifndef SURGE_VK_ATOM_SHADER_HPP
#define SURGE_VK_ATOM_SHADER_HPP

#include "sc_error_types.hpp"
#include "sc_vulkan/sc_vulkan.hpp"

#include <array>
#include <tl/expected.hpp>
#include <vulkan/vulkan.h>

namespace surge::vk_atom::shader {

auto load_shader_module(renderer::vk::context ctx, const char *path) noexcept
    -> tl::expected<VkShaderModule, error>;

void destroy_shader_module(renderer::vk::context ctx, VkShaderModule module) noexcept;

using program_shaders = std::array<VkShaderEXT, 2>;

auto create_shader_object(renderer::vk::context ctx, const char *vertex_shader_path,
                          const char *fragment_shader_path) noexcept
    -> tl::expected<program_shaders, error>;

void destroy_shader_object(renderer::vk::context ctx, program_shaders &shaders) noexcept;

} // namespace surge::vk_atom::shader

#endif // SURGE_VK_ATOM_SHADER_HPP