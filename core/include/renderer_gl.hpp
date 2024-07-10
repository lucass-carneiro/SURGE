#ifndef SURGE_CORE_RENDERER_GL_HPP
#define SURGE_CORE_RENDERER_GL_HPP

#include "config.hpp"
#include "error_types.hpp"

// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on

#include <optional>

namespace surge::renderer {

auto init_opengl(const config::renderer_attrs &r_attrs) noexcept -> std::optional<error>;

} // namespace surge::renderer

#endif // SURGE_CORE_RENDERER_GL_HPP