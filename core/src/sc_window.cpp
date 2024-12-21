#include "sc_window.hpp"

#include "sc_error_types.hpp"
#include "sc_logging.hpp"

#include <gsl/gsl-lite.hpp>

static void glfw_error_callback(int code, const char *description) {
  log_error("GLFW error code {}: {}", code, description);
}

auto surge::window::init(const config::window_resolution &wres, const config::window_attrs &w_attrs,
                         const config::renderer_attrs &r_attrs) -> tl::expected<window_t, error> {
  /*************
   * GLFW init *
   *************/
  log_info("Initializing GLFW");

  glfwSetErrorCallback(glfw_error_callback);

  if (glfwInit() != GLFW_TRUE) {
    return tl::unexpected{error::glfw_init};
  }

  /*****************
   * Monitor query *
   *****************/
  log_info("Querying monitors");

  int mc = 0;
  GLFWmonitor **monitors = glfwGetMonitors(&mc);

  if (monitors == nullptr) {
    glfwTerminate();
    return tl::unexpected{error::glfw_monitor};
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
      return tl::unexpected{error::glfw_monitor_size};
    }

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    glfwGetMonitorContentScale(monitors[i], &xscale, &yscale);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return tl::unexpected{error::glfw_monitor_scale};
    }

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    glfwGetMonitorPos(monitors[i], &xpos, &ypos);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return tl::unexpected{error::glfw_monitor_bounds};
    }

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    glfwGetMonitorWorkarea(monitors[i], &w_xpos, &w_ypos, &w_width, &w_height);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return tl::unexpected{error::glfw_monitor_area};
    }

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const char *name = glfwGetMonitorName(monitors[i]);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return tl::unexpected{error::glfw_monitor_name};
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
      return tl::unexpected{error::glfw_window_hint_major};
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return tl::unexpected{error::glfw_window_hint_minor};
    }

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return tl::unexpected{error::glfw_window_hint_profile};
    }
  } else {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return tl::unexpected{error::glfw_window_hint_api};
    }
  }

  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return tl::unexpected{error::glfw_window_hint_resize};
  }

  window_t engine_window{nullptr};

  if (w_attrs.windowed) {
    engine_window
        = glfwCreateWindow(wres.width, wres.height, w_attrs.name.c_str(), nullptr, nullptr);
  } else {
    engine_window = glfwCreateWindow(
        wres.width, wres.height, w_attrs.name.c_str(),
        monitors[w_attrs.monitor_index < mc ? w_attrs.monitor_index : 0], // NOLINT
        nullptr);
  }

  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return tl::unexpected{error::glfw_window_creation};
  }

  log_info("Engine window created, handle {}", static_cast<void *>(engine_window));

  /*******************************
   *           CURSORS           *
   *******************************/
  log_info("Setting cursor mode for window {}", static_cast<void *>(engine_window));

  glfwSetInputMode(engine_window, GLFW_CURSOR,
                   w_attrs.cursor ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return tl::unexpected{error::glfw_window_input_mode};
  }

  return engine_window;
}

void surge::window::terminate(window_t window) {
  log_info("Terminating window {}", static_cast<void *>(window));
  glfwDestroyWindow(window);
  glfwTerminate();
}

void surge::window::poll_events() { glfwPollEvents(); }

auto surge::window::get_dims(window_t window) -> glm::vec2 {
  int ww{0}, wh{0};
  glfwGetWindowSize(window, &ww, &wh);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to determine window dimentions");
    return glm::vec2{0.0f};
  } else {
    return glm::vec2{gsl::narrow_cast<float>(ww), gsl::narrow_cast<float>(wh)};
  }
}

auto surge::window::get_cursor_pos(window_t window) -> glm::vec2 {
  double x{0}, y{0};
  glfwGetCursorPos(window, &x, &y);

  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to determine window dimentions");
    return glm::vec2{0.0f};
  } else {
    return glm::vec2{static_cast<float>(x), static_cast<float>(y)};
  }
}

auto surge::window::get_key(window_t window, int key) -> int { return glfwGetKey(window, key); }

auto surge::window::get_mouse_button(window_t window, int button) -> int {
  return glfwGetMouseButton(window, button);
}

auto surge::window::should_close(window_t window) -> bool {
  return static_cast<bool>(glfwWindowShouldClose(window));
}

void surge::window::set_should_close(window_t window, bool value) {
  glfwSetWindowShouldClose(window, value ? GLFW_TRUE : GLFW_FALSE);
}

void surge::window::swap_buffers(window_t window) { glfwSwapBuffers(window); }