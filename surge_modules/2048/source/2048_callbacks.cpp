#include "2048.hpp"
#include "2048_piece.hpp"

SURGE_MODULE_EXPORT void update(double) noexcept {
  // do nothing
}

SURGE_MODULE_EXPORT void keyboard_event(GLFWwindow *, int key, int, int action, int mods) noexcept {
  if (key == GLFW_KEY_A && action == GLFW_PRESS && mods == GLFW_MOD_CONTROL) {
    // CTRL + A
    mod_2048::piece::add_random();
  }
}

SURGE_MODULE_EXPORT void mouse_button_event(GLFWwindow *, int, int, int) noexcept {
  // Do nothing
}

SURGE_MODULE_EXPORT void mouse_scroll_event(GLFWwindow *, double, double) noexcept {
  // Do nothing
}