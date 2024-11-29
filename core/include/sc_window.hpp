#ifndef SURGE_CORE_WINDOW_HPP
#define SURGE_CORE_WINDOW_HPP

#include "sc_config.hpp"
#include "sc_glfw_includes.hpp"
#include "sc_glm_includes.hpp"
#include "sc_module.hpp"

#include <optional>

namespace surge::window {

auto init(const config::window_resolution &wres, const config::window_attrs &w_attrs,
          const config::renderer_attrs &r_attrs) -> std::optional<error>;
void terminate();

void poll_events();
auto get_dims() -> glm::vec2;
auto get_cursor_pos() -> glm::vec2;
auto get_key(int key) -> int;
auto get_mouse_button(int button) -> int;
auto should_close() -> bool;
void set_should_close(bool value);

void swap_buffers();

auto get_window_ptr() -> GLFWwindow *;

auto bind_module_input_callbacks(module::api *mod_api) -> std::optional<error>;
void unbind_input_callbacks();

} // namespace surge::window

#endif // SURGE_CORE_WINDOW_HPP