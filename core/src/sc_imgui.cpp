#include "sc_imgui.hpp"

#include "sc_window.hpp"

// clang-format off
#include <imgui_impl_glfw.h>
// clang-format on

void surge::imgui::register_mouse_callback(int button, int action, int mods) noexcept {
  ImGui_ImplGlfw_MouseButtonCallback(window::get_window_ptr(), button, action, mods);
}

void surge::imgui::register_mouse_scroll_callback(double xoffset, double yoffset) noexcept {
  ImGui_ImplGlfw_ScrollCallback(window::get_window_ptr(), xoffset, yoffset);
}
