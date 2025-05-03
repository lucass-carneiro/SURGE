#ifndef SURGE_CORE_WINDOW_HPP
#define SURGE_CORE_WINDOW_HPP

#include "sc_config.hpp"
#include "sc_error_types.hpp"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace surge::window {

using Window = GLFWwindow *;

auto init(const config::WindowResolution &wres,
          const config::WindowAttributes &w_attrs) -> Result<Window>;
void terminate(Window w);

void poll_events();
auto get_dims(Window window) -> glm::vec2;
auto get_cursor_pos(Window window) -> glm::vec2;
auto get_key(Window window, int key) -> int;
auto get_mouse_button(Window window, int button) -> int;
auto should_close(Window window) -> bool;
void set_should_close(Window window, bool value);

} // namespace surge::window

#endif // SURGE_CORE_WINDOW_HPP