#include "window.hpp"

#include "error_types.hpp"
#include "logging.hpp"

#include <gsl/gsl-lite.hpp>

static GLFWwindow *g_engine_window{nullptr};

static void glfw_error_callback(int code, const char *description) noexcept {
  log_error("GLFW error code {}: {}", code, description);
}

static void glfw_framebuffer_size_callback(GLFWwindow *, int width, int height) noexcept {
  glViewport(GLint{0}, GLint{0}, GLsizei{width}, GLsizei{height});
}

// See https://www.khronos.org/opengl/wiki/OpenGL_Error#Catching_errors_.28the_easy_way.29
void GLAPIENTRY gl_error_callback(GLenum, GLenum, GLuint, GLenum severity, GLsizei,
                                  const GLchar *message, const void *) {

  if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
#ifdef SURGE_LOG_GL_NOTIFICATIONS
    log_info("OpenGL info: {}", message);
#endif
  } else if (severity == GL_DEBUG_SEVERITY_LOW || severity == GL_DEBUG_SEVERITY_LOW_ARB) {
    log_warn("OpenGL low severity warning: {}", message);
  } else if (severity == GL_DEBUG_SEVERITY_MEDIUM || severity == GL_DEBUG_SEVERITY_MEDIUM_ARB) {
    log_warn("OpenGL medium severity warning: {}", message);
  } else {
    log_error("OpenGL error: {}", message);
  }
}

auto surge::window::init(const config::window_resolution &wres, const config::window_attrs &w_attrs,
                         const config::renderer_attrs &r_attrs) noexcept -> std::optional<error> {
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

  if (r_attrs.backend == config::renderer_backend::opengl) {
    /***********************
     * OpenGL context init *
     ***********************/
    log_info("Initializing OpenGL");

    glfwMakeContextCurrent(g_engine_window);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return error::glfw_make_ctx;
    }

    if (r_attrs.vsync) {
      log_info("VSync enabled");
      glfwSwapInterval(1);
    } else {
      glfwSwapInterval(0);
      log_info("VSync disabled");
    }

    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return error::glfw_vsync;
    }

    /********
     * GLAD *
     ********/
    log_info("Initializing GLAD");

    // NOLINTNEXTLINE
    if (gladLoadGL() == 0) {
      log_error("Failed to initialize GLAD");
      glfwTerminate();
      return error::glad_loading;
    }

    // Check extension support
    if (!GLAD_GL_ARB_bindless_texture) {
      log_error(
          "SURGE needs an OpenGL implementation that supports bindless textures and the "
          "current implementation does not. Unfortunatelly, SURGE cannot work in this platform "
          "until your graphics card vendor adds this support or it becomes a standardized "
          "OpenGL feature and your vendor produces drivers that support it.");
      glfwTerminate();
      return error::opengl_feature_missing;
    }

    if (!GLAD_GL_ARB_gpu_shader_int64) {
      log_error(
          "SURGE needs an OpenGL implementation that supports int64 in GPU shaders and the "
          "current implementation does not. Unfortunatelly, SURGE cannot work in this platform "
          "until your graphics card vendor adds this support or it becomes a standardized "
          "OpenGL feature and your vendor produces drivers that support it.");
      glfwTerminate();
      return error::opengl_feature_missing;
    }

    // Resize callback
    glfwSetFramebufferSizeCallback(g_engine_window, glfw_framebuffer_size_callback);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return error::glfw_resize_callback;
    }

    /******************
     * OpenGL options *
     ******************/
    // NOLINTNEXTLINE
    log_info("Using OpenGL Version {}", reinterpret_cast<const char *>(glGetString(GL_VERSION)));

#ifdef SURGE_BUILD_TYPE_Debug
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(gl_error_callback, nullptr);
#endif

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // MSAA
    if (r_attrs.MSAA) {
      glfwWindowHint(GLFW_SAMPLES, 4);
      glEnable(GL_MULTISAMPLE);
    }
  }

  return {};
}

void surge::window::terminate() noexcept {
  log_info("Terminating window and renderer");
  glfwDestroyWindow(g_engine_window);
  glfwTerminate();
}

void surge::window::poll_events() noexcept { glfwPollEvents(); }

auto surge::window::get_dims() noexcept -> glm::vec2 {
  int ww{0}, wh{0};
  glfwGetWindowSize(g_engine_window, &ww, &wh);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to determine window dimentions");
    return glm::vec2{0.0f};
  } else {
    return glm::vec2{gsl::narrow_cast<float>(ww), gsl::narrow_cast<float>(wh)};
  }
}

auto surge::window::get_cursor_pos() noexcept -> glm::vec2 {
  double x{0}, y{0};
  glfwGetCursorPos(g_engine_window, &x, &y);

  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to determine window dimentions");
    return glm::vec2{0.0f};
  } else {
    return glm::vec2{static_cast<float>(x), static_cast<float>(y)};
  }
}

auto surge::window::get_key(int key) noexcept -> int { return glfwGetKey(g_engine_window, key); }

auto surge::window::get_mouse_button(int button) noexcept -> int {
  return glfwGetMouseButton(g_engine_window, button);
}

auto surge::window::should_close() noexcept -> bool {
  return static_cast<bool>(glfwWindowShouldClose(g_engine_window));
}

void surge::window::set_should_close(bool value) noexcept {
  glfwSetWindowShouldClose(g_engine_window, value ? GLFW_TRUE : GLFW_FALSE);
}

void surge::window::clear_buffers(const config::clear_color &w_ccl) noexcept {
  glClearColor(w_ccl.r, w_ccl.g, w_ccl.b, w_ccl.a);
  glClear(GL_COLOR_BUFFER_BIT);
  glClear(GL_DEPTH_BUFFER_BIT);
}

void surge::window::swap_buffers() noexcept { glfwSwapBuffers(g_engine_window); }

auto surge::window::set_key_callback(GLFWkeyfun f) noexcept -> std::optional<error> {
  glfwSetKeyCallback(g_engine_window, f);
  auto status{glfwGetError(nullptr)};

  if (status != GLFW_NO_ERROR) {
    log_error("Unable to bind keyboard event callback. GLFW error code {:#x}", status);
    return error::keyboard_event_unbinding;
  } else {
    return {};
  }
}

auto surge::window::set_mouse_button_callback(GLFWmousebuttonfun f) noexcept
    -> std::optional<error> {
  glfwSetMouseButtonCallback(g_engine_window, f);
  auto status{glfwGetError(nullptr)};

  if (status != GLFW_NO_ERROR) {
    log_error("Unable to bind keyboard event callback. GLFW error code {:#x}", status);
    return error::mouse_button_event_unbinding;
  } else {
    return {};
  }
}

auto surge::window::set_mouse_scroll_callback(GLFWscrollfun f) noexcept -> std::optional<error> {
  glfwSetScrollCallback(g_engine_window, f);
  auto status{glfwGetError(nullptr)};

  if (status != GLFW_NO_ERROR) {
    log_error("Unable to bind keyboard event callback. GLFW error code {:#x}", status);
    return error::mouse_scroll_event_unbinding;
  } else {
    return {};
  }
}