#include "lasers.hpp"

// clang-format off
#include "globals.hpp"
// clang-format on

#include "logging.hpp"
#include "renderer.hpp"
#include "window.hpp"

auto surge::mod::lasers::bind_callbacks(GLFWwindow *window) noexcept -> std::uint32_t {
  using namespace surge::mod::lasers;

  log_info("Binding interaction callbacks");

  glfwSetKeyCallback(window, keyboard_event);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to bind keyboard event callback");
    return static_cast<std::uint32_t>(error::keyboard_event_unbinding);
  }

  glfwSetMouseButtonCallback(window, mouse_button_event);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to bind mouse button event callback");
    return static_cast<std::uint32_t>(error::mouse_button_event_unbinding);
  }

  glfwSetScrollCallback(window, mouse_scroll_event);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to bind mouse scroll event callback");
    return static_cast<std::uint32_t>(error::mouse_scroll_event_unbinding);
  }

  return 0;
}

auto surge::mod::lasers::unbind_callbacks(GLFWwindow *window) noexcept -> std::uint32_t {
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
  using namespace surge::mod::lasers;

  const auto bind_callback_stat{bind_callbacks(window)};
  if (bind_callback_stat != 0) {
    return bind_callback_stat;
  }

  log_info("Setting projection matrix");
  std::apply(globals::make_projection, window::get_dims(window));

  log_info("Creating line renderer context");
  const auto make_line_ctx_stat{globals::make_line_ctx()};
  if (make_line_ctx_stat != 0) {
    return make_line_ctx_stat;
  }

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto on_unload(GLFWwindow *window) noexcept -> std::uint32_t {
  using namespace surge::mod::lasers;

  const auto unbind_callback_stat{unbind_callbacks(window)};
  if (unbind_callback_stat != 0) {
    return unbind_callback_stat;
  }

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto draw() noexcept -> std::uint32_t {
  using namespace surge::mod::lasers;
  using namespace surge::renderer;

  const auto &line_ctx{globals::get_line_ctx()};
  const line::draw_context draw_ctx{globals::get_projection(), globals::get_view()};
  line::draw(line_ctx, draw_ctx);
  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto update(double) noexcept -> std::uint32_t { return 0; }

extern "C" SURGE_MODULE_EXPORT void keyboard_event(GLFWwindow *, int, int, int, int) noexcept {}

extern "C" SURGE_MODULE_EXPORT void mouse_button_event(GLFWwindow *, int, int, int) noexcept {}

extern "C" SURGE_MODULE_EXPORT void mouse_scroll_event(GLFWwindow *, double, double) noexcept {}