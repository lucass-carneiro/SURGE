#include "2048.hpp"
#include "2048_debug_window.hpp"
#include "2048_pieces.hpp"

SURGE_MODULE_EXPORT void update(double dt) noexcept {
  using namespace mod_2048;
  pieces::update(dt);
}

SURGE_MODULE_EXPORT void keyboard_event(GLFWwindow *, int key, int, int action, int mods) noexcept {
  using namespace mod_2048;

  if (key == GLFW_KEY_A && action == GLFW_PRESS && mods == GLFW_MOD_CONTROL) {
    pieces::add_random();
  } else if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
    pieces::compress_right();
  }
}

// DearImGui callbacks need to be manually installed. See
// https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_glfw.h

SURGE_MODULE_EXPORT void mouse_button_event(GLFWwindow *window, int button, int action,
                                            int mods) noexcept {
  ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
}

SURGE_MODULE_EXPORT void mouse_scroll_event(GLFWwindow *window, double xoffset,
                                            double yoffset) noexcept {
  ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
}