#ifndef SURGE_CORE_WINDOW_HPP
#define SURGE_CORE_WINDOW_HPP

#include "config.hpp"
#include "renderer.hpp"

#include <glm/glm.hpp>
#include <tl/expected.hpp>

namespace surge::window {

auto init(const config::window_resolution &wres, const config::window_attrs &w_attrs) noexcept
    -> tl::expected<GLFWwindow *, error>;
void terminate(GLFWwindow *window) noexcept;

auto get_dims(GLFWwindow *window) noexcept -> glm::vec2;
auto get_cursor_pos(GLFWwindow *window) noexcept -> glm::vec2;

} // namespace surge::window

#endif // SURGE_CORE_WINDOW_HPP