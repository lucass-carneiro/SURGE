#ifndef SURGE_CORE_RENDERER_HPP
#define SURGE_CORE_RENDERER_HPP

#include "error_types.hpp"
#include "config.hpp"

// clang-format off
#define GLFW_INCLUDE_VULKAN
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
// clang-format on

#include <optional>

namespace surge::renderer {

auto init_opengl(const config::renderer_attrs &r_attrs) noexcept -> std::optional<error>;

} // namespace surge::renderer

#endif // SURGE_CORE_RENDERER_HPP