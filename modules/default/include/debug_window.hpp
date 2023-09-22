#ifndef SURGE_MODULE_DEFAULT_DEBUG_WINDOW_HPP
#define SURGE_MODULE_DEFAULT_DEBUG_WINDOW_HPP

#include "window.hpp"

namespace mod_default::debug_window {

void imgui_init(GLFWwindow *window) noexcept;
void imgui_terminate() noexcept;
void imgui_keyboard_callback(GLFWwindow *window, int button, int action, int mods) noexcept;
void imgui_scroll_callback(GLFWwindow *window, double xoffset, double yoffset) noexcept;

void main_window(bool *p_open = nullptr) noexcept;
void draw() noexcept;

} // namespace mod_default::debug_window

#endif // SURGE_MODULE_DEFAULT_DEBUG_WINDOW_HPP