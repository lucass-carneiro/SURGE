#ifndef SURGE_CORE_WINDOW_HPP
#define SURGE_CORE_WINDOW_HPP

#include "config.hpp"
#include "glfw_includes.hpp"
#include "module.hpp"

#include <optional>

namespace surge::window {

auto init(const config::window_resolution &wres, const config::window_attrs &w_attrs,
          const config::renderer_attrs &r_attrs) noexcept -> std::optional<error>;
void terminate() noexcept;

void poll_events() noexcept;
auto get_dims() noexcept -> glm::vec2;
auto get_cursor_pos() noexcept -> glm::vec2;
auto get_key(int key) noexcept -> int;
auto get_mouse_button(int button) noexcept -> int;
auto should_close() noexcept -> bool;
void set_should_close(bool value) noexcept;

void swap_buffers() noexcept;

auto get_window_ptr() noexcept -> GLFWwindow *;

auto bind_module_input_callbacks(module::api *mod_api) noexcept -> std::optional<error>;
void unbind_input_callbacks() noexcept;

} // namespace surge::window

#endif // SURGE_CORE_WINDOW_HPP