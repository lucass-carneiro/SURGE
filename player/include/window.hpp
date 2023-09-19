#ifndef SURGE_WINDOW_HPP
#define SURGE_WINDOW_HPP

#include "config.hpp"
#include "renderer.hpp"

#include <GLFW/glfw3.h>
#include <tl/expected.hpp>

namespace surge::window {

enum class window_error {
  glfw_init,
  glfw_monitor,
  glfw_monitor_size,
  glfw_monitor_scale,
  glfw_monitor_bounds,
  glfw_monitor_area,
  glfw_monitor_name,
  glfw_window_hint_major,
  glfw_window_hint_minor,
  glfw_window_hint_profile,
  glfw_window_hint_resize,
  glfw_window_creation,
  glfw_window_input_mode,
  glfw_make_ctx,
  glfw_vsync,
  glfw_resize_callback,
  glad_loading,
};

auto init(const config::window_resolution &wres, const config::window_attrs &w_attrs) noexcept
    -> tl::expected<GLFWwindow *, window_error>;
void terminate(GLFWwindow *window) noexcept;

auto get_dims(GLFWwindow *window) noexcept -> std::tuple<float, float>;

} // namespace surge::window

#endif // SURGE_WINDOW_HPP