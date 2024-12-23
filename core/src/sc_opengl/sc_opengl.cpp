#include "sc_opengl/sc_opengl.hpp"

#include "sc_logging.hpp"
#include "sc_options.hpp"
#include "sc_window.hpp"

#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
#  include <tracy/TracyOpenGL.hpp>
#endif

static void glfw_gl_framebuffer_size_callback(GLFWwindow *, int width, int height) noexcept {
  glViewport(GLint{0}, GLint{0}, GLsizei{width}, GLsizei{height});
}

// See https://www.khronos.org/opengl/wiki/OpenGL_Error#Catching_errors_.28the_easy_way.29
#ifdef SURGE_GL_LOG
static void GLAPIENTRY gl_error_callback(GLenum, GLenum, GLuint, GLenum severity, GLsizei,
                                         const GLchar *message, const void *) {

  if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
#  ifdef SURGE_LOG_GL_NOTIFICATIONS
    log_info("OpenGL info: {}", message);
#  endif
  } else if (severity == GL_DEBUG_SEVERITY_LOW || severity == GL_DEBUG_SEVERITY_LOW_ARB) {
    log_warn("OpenGL low severity warning: {}", message);
  } else if (severity == GL_DEBUG_SEVERITY_MEDIUM || severity == GL_DEBUG_SEVERITY_MEDIUM_ARB) {
    log_warn("OpenGL medium severity warning: {}", message);
  } else {
    log_error("OpenGL error: {}", message);
  }
}
#endif

auto surge::renderer::gl::init(window::window_t w, const config::renderer_attrs &r_attrs) noexcept
    -> std::optional<error> {
  /***********************
   * OpenGL context init *
   ***********************/
  log_info("Initializing OpenGL");

  glfwMakeContextCurrent(w);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return error::glfw_make_ctx;
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
    log_error("SURGE needs an OpenGL implementation that supports bindless textures and the "
              "current implementation does not. Unfortunatelly, SURGE cannot work in this platform "
              "until your graphics card vendor adds this support or it becomes a standardized "
              "OpenGL feature and your vendor produces drivers that support it.");
    glfwTerminate();
    return error::opengl_feature_missing;
  }

  if (!GLAD_GL_ARB_gpu_shader_int64) {
    log_error("SURGE needs an OpenGL implementation that supports int64 in GPU shaders and the "
              "current implementation does not. Unfortunatelly, SURGE cannot work in this platform "
              "until your graphics card vendor adds this support or it becomes a standardized "
              "OpenGL feature and your vendor produces drivers that support it.");
    glfwTerminate();
    return error::opengl_feature_missing;
  }

  // Resize callback
  glfwSetFramebufferSizeCallback(w, glfw_gl_framebuffer_size_callback);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    glfwTerminate();
    return error::glfw_resize_callback;
  }

  /******************
   * OpenGL options *
   ******************/
  // NOLINTNEXTLINE
  log_info("Using OpenGL Version {}", reinterpret_cast<const char *>(glGetString(GL_VERSION)));

  if (r_attrs.vsync) {
    log_info("VSync enabled");
    glfwSwapInterval(1);
  } else {
    glfwSwapInterval(0);
    log_info("VSync disabled");
  }

  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_error("Unable to set VSync");
    glfwTerminate();
    return error::glfw_vsync;
  }

#ifdef SURGE_GL_LOG
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

#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
  TracyGpuContext;
#endif

  return {};
}

void surge::renderer::gl::wait_idle() noexcept {
  glFlush();
  glFinish();
}

void surge::renderer::gl::clear(const config::clear_color &w_ccl) noexcept {
  glClearColor(w_ccl.r, w_ccl.g, w_ccl.b, w_ccl.a);
  glClear(GL_COLOR_BUFFER_BIT);
  glClear(GL_DEPTH_BUFFER_BIT);
}