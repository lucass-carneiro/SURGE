#include "default.hpp"

auto mod_default::bind_callbacks() noexcept -> int {
  using namespace surge;

  log_info("Binding interaction callbacks");

  auto status{window::set_key_callback(keyboard_event)};

  if (status.has_value()) {
    log_error("Unable to bind keyboard event callback");
    return static_cast<int>(status.value());
  }

  status = window::set_mouse_button_callback(mouse_button_event);

  if (status.has_value()) {
    log_error("Unable to bind mouse button event callback.");
    return static_cast<int>(status.value());
  }

  status = window::set_mouse_scroll_callback(mouse_scroll_event);

  if (status.has_value()) {
    log_error("Unable to bind mouse scroll event callback");
    return static_cast<int>(status.value());
  }

  return 0;
}

auto mod_default::unbind_callbacks() noexcept -> int {
  using namespace surge;

  log_info("Unbinding interaction callbacks");

  auto status{window::set_key_callback(nullptr)};

  if (status.has_value()) {
    log_error("Unable to bind keyboard event callback");
  }

  status = window::set_mouse_button_callback(nullptr);

  if (status.has_value()) {
    log_error("Unable to bind mouse button event callback.");
  }

  status = window::set_mouse_scroll_callback(nullptr);

  if (status.has_value()) {
    log_error("Unable to bind mouse scroll event callback");
  }

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto on_load() noexcept -> int {
  using namespace mod_default;

  const auto bind_callback_stat{bind_callbacks()};
  if (bind_callback_stat != 0) {
    return bind_callback_stat;
  }

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto on_unload() noexcept -> int {
  using namespace mod_default;

  const auto unbind_callback_stat{unbind_callbacks()};
  if (unbind_callback_stat != 0) {
    return unbind_callback_stat;
  }

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto draw() noexcept -> int { return 0; }

extern "C" SURGE_MODULE_EXPORT auto update(double) noexcept -> int { return 0; }

extern "C" SURGE_MODULE_EXPORT void keyboard_event(GLFWwindow*, int, int, int, int) noexcept {}

extern "C" SURGE_MODULE_EXPORT void mouse_button_event(GLFWwindow*, int, int, int) noexcept {}

extern "C" SURGE_MODULE_EXPORT void mouse_scroll_event(GLFWwindow*, double, double) noexcept {}