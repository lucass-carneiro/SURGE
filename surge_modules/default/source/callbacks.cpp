#include "default.hpp"

SURGE_MODULE_EXPORT void update(double) noexcept {
  // do nothing
}

SURGE_MODULE_EXPORT void keyboard_event(GLFWwindow *, int, int, int, int) noexcept {
  // Do nothing
}

SURGE_MODULE_EXPORT void mouse_button_event(GLFWwindow *, int, int, int) noexcept {
  // Do nothing
}

SURGE_MODULE_EXPORT void mouse_scroll_event(GLFWwindow *, double, double) noexcept {
  // Do nothing
}