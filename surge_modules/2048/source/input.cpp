#include "2048.hpp"
#include "logging.hpp"

extern "C" {

SURGE_MODULE_EXPORT void draw() noexcept {
  // log_info("draw");
  // TODO
}

SURGE_MODULE_EXPORT void update(double) noexcept {
  // log_info("update");
  // TODO
}

SURGE_MODULE_EXPORT void keyboard_event(GLFWwindow *, int, int, int, int) noexcept {
  log_info("keyboard callback");
  // TODO
}

SURGE_MODULE_EXPORT void mouse_button_event(GLFWwindow *, int, int, int) noexcept {
  log_info("mouse button callback");
  // TODO
}

SURGE_MODULE_EXPORT void mouse_scroll_event(GLFWwindow *, double, double) noexcept {
  log_info("mouse scroll callback");
  // TODO
}
}