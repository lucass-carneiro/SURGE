#ifndef SURGE_MODULE_2048_DEBUG_WINDOW_HPP
#define SURGE_MODULE_2048_DEBUG_WINDOW_HPP

// clang-format off
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
// clang-format on

namespace mod_2048::debug_window {

void imgui_init(GLFWwindow *window) noexcept;
void imgui_terminate() noexcept;
void imgui_keyboard_callback(GLFWwindow *window, int button, int action, int mods) noexcept;
void imgui_scroll_callback(GLFWwindow *window, double xoffset, double yoffset) noexcept;

void main_window(bool *p_open = nullptr) noexcept;
void draw() noexcept;

} // namespace mod_2048::debug_window

#endif // SURGE_MODULE_2048_DEBUG_WINDOW_HPP