#ifndef SURGE_CORE_WINDOW_HPP
#define SURGE_CORE_WINDOW_HPP

#include "config.hpp"
#include "renderer.hpp"

#include <glm/glm.hpp>
#include <optional>

namespace surge::window {

auto init(const config::window_resolution &wres, const config::window_attrs &w_attrs) noexcept
    -> std::optional<error>;
void terminate() noexcept;

void poll_events() noexcept;
auto get_dims() noexcept -> glm::vec2;
auto get_cursor_pos() noexcept -> glm::vec2;
auto get_key(int key) noexcept -> int;
auto should_close() noexcept -> bool;
void set_should_close(bool value) noexcept;

auto set_key_callback(GLFWkeyfun f) noexcept -> std::optional<error>;
auto set_mouse_button_callback(GLFWmousebuttonfun f) noexcept -> std::optional<error>;
auto set_mouse_scroll_callback(GLFWscrollfun f) noexcept -> std::optional<error>;

} // namespace surge::window

#endif // SURGE_CORE_WINDOW_HPP