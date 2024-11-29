#include "sc_window.hpp"

#include "sc_error_types.hpp"
#include "sc_logging.hpp"

#include <gsl/gsl-lite.hpp>

static GLFWwindow *g_engine_window{nullptr};

static void glfw_error_callback(int code, const char *description) {
  log_error("GLFW error code {}: {}", code, description);
}

auto surge::window::init(const config::window_resolution &wres, const config::window_attrs &w_attrs,
                         const config::renderer_attrs &r_attrs) -> std::optional<error> {
  /*************
   * GLFW init *
   *************/
  log_info("Initializing GLFW");

  glfwSetErrorCallback(glfw_error_callback);

  if (glfwInit() != GLFW_TRUE) {
    return error::glfw_init;
  }

  /*****************
   * Monitor query *
   *****************/
  log_info("Querying monitors");

  int mc = 0;
  GLFWmonitor **monitors = glfwGetMonitors(&mc);

  if (monitors == nullptr) {
    glfwTerminate();
    return error::glfw_monitor;
  }

  log_info("Monitors detected: {}", mc);

  for (int i = 0; i < mc; i++) {
    int width = 0, height = 0;
    float xscale = 0, yscale = 0;
    int xpos = 0, ypos = 0;
    int w_xpos = 0, w_ypos = 0, w_width = 0, w_height = 0;

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    glfwGetMonitorPhysicalSize(monitors[i], &width, &height);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return error::glfw_monitor_size;
    }

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    glfwGetMonitorContentScale(monitors[i], &xscale, &yscale);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return error::glfw_monitor_scale;
    }

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    glfwGetMonitorPos(monitors[i], &xpos, &ypos);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return error::glfw_monitor_bounds;
    }

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    glfwGetMonitorWorkarea(monitors[i], &w_xpos, &w_ypos, &w_width, &w_height);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return error::glfw_monitor_area;
    }

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const char *name = glfwGetMonitorName(monitors[i]);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return error::glfw_monitor_name;
    }

    log_info("Properties of monitor {}:\n"
             "  Monitor name: {}.\n"
             "  Physical size (width, height): {}, {}.\n"
             "  Content scale (x, y): {}, {}.\n"
             "  Virtual position: (x, y): {}, {}.\n"
             "  Work area (x, y, width, height): {}, {}, {}, {}.",
             i, name, width, height, xscale, yscale, xpos, ypos, w_xpos, w_ypos, w_width, w_height);
  }

  /***************
   * Window init *
   ***************/
  log_info("Initializing engine window");

  if (r_attrs.backend == config::renderer_backend::opengl) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return error::glfw_window_hint_major;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return error::glfw_window_hint_minor;
    }

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return error::glfw_window_hint_profile;
    }
  } else {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return error::glfw_window_hint_api;
    }
  }

  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return error::glfw_window_hint_resize;
  }

  if (w_attrs.windowed) {
    g_engine_window
        = glfwCreateWindow(wres.width, wres.height, w_attrs.name.c_str(), nullptr, nullptr);
  } else {
    g_engine_window = glfwCreateWindow(
        wres.width, wres.height, w_attrs.name.c_str(),
        monitors[w_attrs.monitor_index < mc ? w_attrs.monitor_index : 0], // NOLINT
        nullptr);
  }

  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return error::glfw_window_creation;
  }

  log_info("Engine window created");

  /*******************************
   *           CURSORS           *
   *******************************/
  log_info("Setting cursor mode");

  glfwSetInputMode(g_engine_window, GLFW_CURSOR,
                   w_attrs.cursor ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return error::glfw_window_input_mode;
  }

  return {};
}

void surge::window::terminate() {
  log_info("Terminating window and renderer");
  glfwDestroyWindow(g_engine_window);
  glfwTerminate();
}

void surge::window::poll_events() { glfwPollEvents(); }

auto surge::window::get_dims() -> glm::vec2 {
  int ww{0}, wh{0};
  glfwGetWindowSize(g_engine_window, &ww, &wh);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to determine window dimentions");
    return glm::vec2{0.0f};
  } else {
    return glm::vec2{gsl::narrow_cast<float>(ww), gsl::narrow_cast<float>(wh)};
  }
}

auto surge::window::get_cursor_pos() -> glm::vec2 {
  double x{0}, y{0};
  glfwGetCursorPos(g_engine_window, &x, &y);

  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to determine window dimentions");
    return glm::vec2{0.0f};
  } else {
    return glm::vec2{static_cast<float>(x), static_cast<float>(y)};
  }
}

auto surge::window::get_key(int key) -> int { return glfwGetKey(g_engine_window, key); }

auto surge::window::get_mouse_button(int button) -> int {
  return glfwGetMouseButton(g_engine_window, button);
}

auto surge::window::should_close() -> bool {
  return static_cast<bool>(glfwWindowShouldClose(g_engine_window));
}

void surge::window::set_should_close(bool value) {
  glfwSetWindowShouldClose(g_engine_window, value ? GLFW_TRUE : GLFW_FALSE);
}

void surge::window::swap_buffers() { glfwSwapBuffers(g_engine_window); }

auto surge::window::get_window_ptr() -> GLFWwindow * { return g_engine_window; }

static void glfw_keyboard_event(GLFWwindow *window, int key, int scancode, int action, int mods) {
  auto usr_ptr{glfwGetWindowUserPointer(window)};
  auto status{glfwGetError(nullptr)};

  if (status != GLFW_NO_ERROR) {
    log_error("Unable to retrieve module api from GLFW user pointer: {:#x}", status);
  } else if (usr_ptr != nullptr) {
    auto mod_api{static_cast<surge::module::api *>(usr_ptr)};
    mod_api->keyboard_event(key, scancode, action, mods);
  }
}

static void glfw_mouse_button_event(GLFWwindow *window, int button, int action, int mods) {
  auto usr_ptr{glfwGetWindowUserPointer(window)};
  auto status{glfwGetError(nullptr)};

  if (status != GLFW_NO_ERROR) {
    log_error("Unable to retrieve module api from GLFW user pointer: {:#x}", status);
  } else if (usr_ptr != nullptr) {
    auto mod_api{static_cast<surge::module::api *>(usr_ptr)};
    mod_api->mouse_button_event(button, action, mods);
  }
}

static void glfw_mouse_scroll_event(GLFWwindow *window, double xoffset, double yoffset) {
  auto usr_ptr{glfwGetWindowUserPointer(window)};
  auto status{glfwGetError(nullptr)};

  if (status != GLFW_NO_ERROR) {
    log_error("Unable to retrieve module api from GLFW user pointer: {:#x}", status);
  } else if (usr_ptr != nullptr) {
    auto mod_api{static_cast<surge::module::api *>(usr_ptr)};
    mod_api->mouse_scroll_event(xoffset, yoffset);
  }
}

auto surge::window::bind_module_input_callbacks(module::api *mod_api) -> std::optional<error> {
  log_info("Binding module interaction callbacks");

  // Set module api as user ptr
  glfwSetWindowUserPointer(g_engine_window, static_cast<void *>(mod_api));
  auto status{glfwGetError(nullptr)};

  if (status != GLFW_NO_ERROR) {
    log_error("Unable to set module api as GLFW user pointer: {:#x}", status);
    return error::glfw_set_usr_ptr;
  }

  // Set Keyboard callback
  glfwSetKeyCallback(g_engine_window, glfw_keyboard_event);
  status = glfwGetError(nullptr);

  if (status != GLFW_NO_ERROR) {
    log_error("Unable to bind keyboard event callback. GLFW error code {:#x}", status);
    return error::keyboard_event_unbinding;
  }

  // Set Mouse button callback
  glfwSetMouseButtonCallback(g_engine_window, glfw_mouse_button_event);
  status = glfwGetError(nullptr);

  if (status != GLFW_NO_ERROR) {
    log_error("Unable to bind keyboard event callback. GLFW error code {:#x}", status);
    return error::mouse_button_event_unbinding;
  }

  // Set mouse scroll callback
  glfwSetScrollCallback(g_engine_window, glfw_mouse_scroll_event);
  status = glfwGetError(nullptr);

  if (status != GLFW_NO_ERROR) {
    log_error("Unable to bind keyboard event callback. GLFW error code {:#x}", status);
    return error::mouse_scroll_event_unbinding;
  }

  return {};
}

void surge::window::unbind_input_callbacks() {
  log_info("Unbinding module interaction callbacks");

  // Set moduel api as user ptr
  glfwSetWindowUserPointer(g_engine_window, nullptr);

  // Set Keyboard callback
  glfwSetKeyCallback(g_engine_window, nullptr);

  // Set Mouse button callback
  glfwSetMouseButtonCallback(g_engine_window, nullptr);

  // Set mouse scroll callback
  glfwSetScrollCallback(g_engine_window, nullptr);
}