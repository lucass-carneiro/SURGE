#include "2048.hpp"

#include "board.hpp"
#include "debug_window.hpp"
#include "logging.hpp"
#include "pieces.hpp"

extern "C" SURGE_MODULE_EXPORT auto draw() noexcept -> std::uint32_t {
  using namespace mod_2048;

  board::draw();
  pieces::draw();
  debug_window::draw();

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto update(double dt) noexcept -> std::uint32_t {
  using namespace mod_2048;
  pieces::update(dt);

  return 0;
}

extern "C" SURGE_MODULE_EXPORT void keyboard_event(GLFWwindow *, int key, int, int action,
                                                   int mods) noexcept {
  using namespace mod_2048;

  if (key == GLFW_KEY_A && action == GLFW_PRESS && mods == GLFW_MOD_CONTROL) {
    pieces::add_random();
  } else if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
    pieces::compress_right();
  }
}

extern "C" SURGE_MODULE_EXPORT void mouse_button_event(GLFWwindow *window, int button, int action,
                                                       int mods) noexcept {
  ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
}

extern "C" SURGE_MODULE_EXPORT void mouse_scroll_event(GLFWwindow *window, double xoffset,
                                                       double yoffset) noexcept {
  ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
}