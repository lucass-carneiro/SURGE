#include "2048.hpp"

#include "logging.hpp"

#ifdef SURGE_BUILD_TYPE_Debug
#  include "debug_window.hpp"
#endif

// clang-format off
#include <GLFW/glfw3.h>
// clang-format on

auto mod_2048::bind_callbacks(GLFWwindow *window) noexcept -> std::uint32_t {
  log_info("Binding interaction callbacks");

  glfwSetKeyCallback(window, keyboard_event);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_debug("Unable to bind keyboard event callback");
    return static_cast<std::uint32_t>(mod_2048::error::keyboard_event_unbinding);
  }

  glfwSetMouseButtonCallback(window, mouse_button_event);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_debug("Unable to bind mouse button event callback");
    return static_cast<std::uint32_t>(mod_2048::error::mouse_button_event_unbinding);
  }

  glfwSetScrollCallback(window, mouse_scroll_event);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_debug("Unable to bind mouse scroll event callback");
    return static_cast<std::uint32_t>(mod_2048::error::mouse_scroll_event_unbinding);
  }

  return 0;
}

auto mod_2048::unbind_callbacks(GLFWwindow *window) noexcept -> std::uint32_t {
  log_info("Unbinding interaction callbacks");

  glfwSetKeyCallback(window, nullptr);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_debug("Unable to unbind keyboard event callback");
  }

  glfwSetMouseButtonCallback(window, nullptr);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_debug("Unable to unbind mouse button event callback");
  }

  glfwSetScrollCallback(window, nullptr);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_debug("Unable to unbind mouse scroll event callback");
  }

  return 0;
}

void mouse_button_event(GLFWwindow *window, int button, int action, int mods) noexcept {
  // New Game button click
  double xpos{0}, ypos{0};
  glfwGetCursorPos(window, &xpos, &ypos);
  if (mod_2048::inside_new_game_button(xpos, ypos) && button == GLFW_MOUSE_BUTTON_LEFT
      && action == GLFW_PRESS) {
    log_debug("Starting new game");
    mod_2048::new_game();
  }

// ImGui window clicks
#ifdef SURGE_BUILD_TYPE_Debug
  ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
#endif
}

#ifdef SURGE_BUILD_TYPE_Debug

void mouse_scroll_event(GLFWwindow *window, double xoffset, double yoffset) noexcept {
  ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
}

#else

void mouse_scroll_event(GLFWwindow *, double, double) noexcept {
  // do nothing
}

#endif