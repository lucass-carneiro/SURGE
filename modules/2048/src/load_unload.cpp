#include "2048.hpp"

// clang-format off
#include "globals.hpp"
#include "board.hpp"
#include "pieces.hpp"
#include "debug_window.hpp"
// clang-format on

#include "logging.hpp"
#include "renderer.hpp"
#include "window.hpp"

auto mod_2048::bind_callbacks(GLFWwindow *window) noexcept -> std::uint32_t {
  log_info("Binding interaction callbacks");

  glfwSetKeyCallback(window, keyboard_event);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to bind keyboard event callback");
    return static_cast<std::uint32_t>(mod_2048::error::keyboard_event_unbinding);
  }

  glfwSetMouseButtonCallback(window, mouse_button_event);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to bind mouse button event callback");
    return static_cast<std::uint32_t>(mod_2048::error::mouse_button_event_unbinding);
  }

  glfwSetScrollCallback(window, mouse_scroll_event);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to bind mouse scroll event callback");
    return static_cast<std::uint32_t>(mod_2048::error::mouse_scroll_event_unbinding);
  }

  return 0;
}

auto mod_2048::unbind_callbacks(GLFWwindow *window) noexcept -> std::uint32_t {
  log_info("Unbinding interaction callbacks");

  glfwSetKeyCallback(window, nullptr);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to unbind keyboard event callback");
  }

  glfwSetMouseButtonCallback(window, nullptr);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to unbind mouse button event callback");
  }

  glfwSetScrollCallback(window, nullptr);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to unbind mouse scroll event callback");
  }

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto on_load(GLFWwindow *window) noexcept -> std::uint32_t {
  using namespace surge;
  using namespace mod_2048;

  const auto bind_callback_stat{bind_callbacks(window)};
  if (bind_callback_stat != 0) {
    return bind_callback_stat;
  }

  debug_window::imgui_init(window);

  log_info("Loading 2048 data");

  log_info("Setting projection matrix");
  std::apply(globals::make_projection, window::get_dims(window));

  log_info("Loading board image");
  if (!board::make_img_ctx()) {
    return static_cast<std::uint32_t>(error::board_image_context);
  }

  log_info("Loading piece image");
  if (!pieces::make_img_ctx()) {
    return static_cast<std::uint32_t>(error::piece_image_context);
  }

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto on_unload(GLFWwindow *window) noexcept -> std::uint32_t {
  using namespace mod_2048;

  const auto unbind_callback_stat{unbind_callbacks(window)};
  if (unbind_callback_stat != 0) {
    return unbind_callback_stat;
  }

  debug_window::imgui_terminate();

  log_info("Unloading 2048 data");
  return 0;
}