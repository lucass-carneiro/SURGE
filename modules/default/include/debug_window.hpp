#ifndef SURGE_MODULE_DEFAULT_DEBUG_WINDOW_HPP
#define SURGE_MODULE_DEFAULT_DEBUG_WINDOW_HPP

// clang-format off
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
// clang-format on

namespace mod_default::debug_window {

void main_window(bool *p_open = nullptr) noexcept;
void draw() noexcept;

} // namespace mod_default::debug_window

#endif // SURGE_MODULE_DEFAULT_DEBUG_WINDOW_HPP