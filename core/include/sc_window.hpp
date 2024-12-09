#ifndef SURGE_CORE_WINDOW_HPP
#define SURGE_CORE_WINDOW_HPP

#include "sc_config.hpp"
#include "sc_glfw_includes.hpp"
#include "sc_glm_includes.hpp"
#include "sc_module.hpp"

#include <tl/expected.hpp>

namespace surge::window {

auto init(const config::window_resolution &wres, const config::window_attrs &w_attrs,
          const config::renderer_attrs &r_attrs) -> tl::expected<window_t, error>;
void terminate(window_t w);

void poll_events();
auto get_dims(window_t window) -> glm::vec2;
auto get_cursor_pos(window_t window) -> glm::vec2;
auto get_key(window_t window, int key) -> int;
auto get_mouse_button(window_t window, int button) -> int;
auto should_close(window_t window) -> bool;
void set_should_close(window_t window, bool value);

void swap_buffers(window_t window);

void unbind_input_callbacks(window_t window);

} // namespace surge::window

#endif // SURGE_CORE_WINDOW_HPP