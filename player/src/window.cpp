// clang-format off
// clang-format on

#include "window.hpp"

#include "logging.hpp"
#include "options.hpp"

#include <gsl/gsl-lite.hpp>

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
#  include <tracy/TracyOpenGL.hpp>
#endif

// Avoid using integrated graphics
#ifdef SURGE_SYSTEM_Windows
extern "C" {
   __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
   __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}
#endif

static void glfw_error_callback(int code, const char *description) noexcept {
  log_error("GLFW erro code %i: %s", code, description);
}

static void glfw_framebuffer_size_callback(GLFWwindow *, int width, int height) noexcept {
  glViewport(GLint{0}, GLint{0}, GLsizei{width}, GLsizei{height});
}

// See https://www.khronos.org/opengl/wiki/OpenGL_Error#Catching_errors_.28the_easy_way.29
void GLAPIENTRY gl_error_callback(GLenum, GLenum, GLuint, GLenum severity, GLsizei,
                                  const GLchar *message, const void *) {

  if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
    log_info("OpenGL info: %s", message);
  } else if (severity == GL_DEBUG_SEVERITY_LOW || GL_DEBUG_SEVERITY_MEDIUM) {
    log_warn("OpenGL warning: %s", message);
  } else {
    log_error("OpenGL error: %s", message);
  }
}

auto surge::window::init(const config::window_resolution &wres,
                         const config::window_attrs &w_attrs) noexcept
    -> tl::expected<GLFWwindow *, error> {
  /*************
   * GLFW init *
   *************/
  log_info("Initializing GLFW");

  glfwSetErrorCallback(glfw_error_callback);

  if (glfwInit() != GLFW_TRUE) {
    return tl::unexpected(error::glfw_init);
  }

  /*****************
   * Monitor query *
   *****************/
  log_info("Querying monitors");

  int mc = 0;
  GLFWmonitor **monitors = glfwGetMonitors(&mc);

  if (monitors == nullptr) {
    glfwTerminate();
    return tl::unexpected(error::glfw_monitor);
  }

  log_info("Monitors detected: %i", mc);

  for (int i = 0; i < mc; i++) {
    int width = 0, height = 0;
    float xscale = 0, yscale = 0;
    int xpos = 0, ypos = 0;
    int w_xpos = 0, w_ypos = 0, w_width = 0, w_height = 0;

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    glfwGetMonitorPhysicalSize(monitors[i], &width, &height);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return tl::unexpected(error::glfw_monitor_size);
    }

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    glfwGetMonitorContentScale(monitors[i], &xscale, &yscale);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return tl::unexpected(error::glfw_monitor_scale);
    }

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    glfwGetMonitorPos(monitors[i], &xpos, &ypos);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return tl::unexpected(error::glfw_monitor_bounds);
    }

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    glfwGetMonitorWorkarea(monitors[i], &w_xpos, &w_ypos, &w_width, &w_height);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return tl::unexpected(error::glfw_monitor_area);
    }

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const char *name = glfwGetMonitorName(monitors[i]);
    if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
      glfwTerminate();
      return tl::unexpected(error::glfw_monitor_name);
    }

    log_info("Properties of monitor %i:\n"
             "  Monitor name: %s.\n"
             "  Physical size (width, height): %i, %i.\n"
             "  Content scale (x, y): %f, %f.\n"
             "  Virtual position: (x, y): %i, %i.\n"
             "  Work area (x, y, width, height): %i, %i, %i, %i.",
             i, name, width, height, xscale, yscale, xpos, ypos, w_xpos, w_ypos, w_width, w_height);
  }

  /***************
   * Window init *
   ***************/
  log_info("Initializing engine window");

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return tl::unexpected(error::glfw_window_hint_major);
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return tl::unexpected(error::glfw_window_hint_minor);
  }

  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return tl::unexpected(error::glfw_window_hint_profile);
  }

#ifdef SURGE_SYSTEM_MacOSX
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return std::make_tuple(nullptr, 0, 0, BGFX_RESET_NONE);
  }
#endif

  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return tl::unexpected(error::glfw_window_hint_resize);
  }

  GLFWwindow *window = nullptr;

  if (w_attrs.windowed) {
    window = glfwCreateWindow(wres.width, wres.height, w_attrs.name.c_str(), nullptr, nullptr);
  } else {
    window = glfwCreateWindow(
        wres.width, wres.height, w_attrs.name.c_str(),
        monitors[w_attrs.monitor_index < mc ? w_attrs.monitor_index : 0], // NOLINT
        nullptr);
  }

  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return tl::unexpected(error::glfw_window_creation);
  }

  log_info("Engine window created");

  /*******************************
   *           CURSORS           *
   *******************************/
  log_info("Setting cursor mode");

  glfwSetInputMode(window, GLFW_CURSOR, w_attrs.cursor ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return tl::unexpected(error::glfw_window_input_mode);
  }

  /***********************
   * OpenGL context init *
   ***********************/
  log_info("Initializing OpenGL");

  glfwMakeContextCurrent(window);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return tl::unexpected(error::glfw_make_ctx);
  }

  if (w_attrs.vsync) {
    log_info("VSync enabled");
    glfwSwapInterval(1);
  } else {
    glfwSwapInterval(0);
    log_info("VSync disabled");
  }

  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return tl::unexpected(error::glfw_vsync);
  }

  /********
   * GLAD *
   ********/
  log_info("Initializing GLAD");

  // NOLINTNEXTLINE
  if (gladLoadGL(glfwGetProcAddress) == 0) {
    log_error("Failed to initialize GLAD");
    glfwTerminate();
    return tl::unexpected(error::glad_loading);
  }

  // Check extension support
  if (!GLAD_GL_ARB_bindless_texture) {
    log_error("SURGE needs an OpenGL implementation that supports bindless textures and the "
              "current implementation does not. Unfortunatelly, SURGE cannot work in this platform "
              "until your graphics card vendor adds this support or it becomes a standardized "
              "OpenGL feature and your vendor produces drivers that support it.");
    glfwTerminate();
    return tl::unexpected(error::opengl_feature_missing);
  }

  if (!GLAD_GL_ARB_gpu_shader_int64) {
    log_error("SURGE needs an OpenGL implementation that supports int64 in GPU shaders and the "
              "current implementation does not. Unfortunatelly, SURGE cannot work in this platform "
              "until your graphics card vendor adds this support or it becomes a standardized "
              "OpenGL feature and your vendor produces drivers that support it.");
    glfwTerminate();
    return tl::unexpected(error::opengl_feature_missing);
  }

  // Resize callback
  glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return tl::unexpected(error::glfw_resize_callback);
  }

  /******************
   * OpenGL options *
   ******************/
  log_info("Using OpenGL Version %s", glGetString(GL_VERSION));

#ifdef SURGE_BUILD_TYPE_Debug
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(gl_error_callback, nullptr);
#endif

  renderer::enable(renderer::capability::depth_test);
  renderer::enable(renderer::capability::blend);
  renderer::blend_function(renderer::blend_src::alpha, renderer::blend_dest::one_minus_src_alpha);

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  glfwGetCurrentContext();
  TracyGpuContext;
#endif

  return window;
}

void surge::window::terminate(GLFWwindow *window) noexcept {
  log_info("Terminating window and renderer");
  glfwDestroyWindow(window);
  glfwTerminate();
}

auto surge::window::get_dims(GLFWwindow *window) noexcept -> std::tuple<float, float> {
  int ww{0}, wh{0};
  glfwGetWindowSize(window, &ww, &wh);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to determine window dimentions");
    return std::make_tuple(0.0f, 0.0f);
  } else {
    return std::make_tuple(gsl::narrow_cast<float>(ww), gsl::narrow_cast<float>(wh));
  }
}

auto surge::window::get_cursor_pos(GLFWwindow *window) noexcept -> glm::vec2 {
  double x{0}, y{0};
  glfwGetCursorPos(window, &x, &y);

  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to determine window dimentions");
    return glm::vec2{0.0f};
  } else {
    return glm::vec2{static_cast<float>(x), static_cast<float>(y)};
  }
}