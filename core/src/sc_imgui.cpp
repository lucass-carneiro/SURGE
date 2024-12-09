#include "sc_imgui.hpp"

#include "sc_window.hpp"

// clang-format off
#include <imgui_impl_glfw.h>
// clang-format on

void surge::imgui::mouse_callback(window::window_t w, int button, int action, int mods) noexcept {
  ImGui_ImplGlfw_MouseButtonCallback(w, button, action, mods);
}

void surge::imgui::mouse_scroll_callback(window::window_t w, double xoffset,
                                         double yoffset) noexcept {
  ImGui_ImplGlfw_ScrollCallback(w, xoffset, yoffset);
}
