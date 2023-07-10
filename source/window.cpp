#include "window.hpp"

#include "logging_system/logging_system.hpp"
#include "options.hpp"
#include "safe_ops.hpp"

// clang-format off
#include "opengl/program.hpp"
#include "opengl/uniforms.hpp"

#include <glm/gtc/matrix_transform.hpp>
// clang-format on

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <gsl/gsl-lite.hpp>
#include <iostream>
#include <sstream>

surge::engine_window::window_ptr_t surge::engine_window::window
    = window_ptr_t{nullptr, glfwDestroyWindow};

GLuint surge::default_shaders::sprite_shader = 0;
GLuint surge::default_shaders::image_shader = 0;

auto surge::engine_window::init(const lua_engine_config &engine_config) noexcept -> bool {
  /*******************************
   *     Initializing GLFW       *
   *******************************/
  log_info("Initializing GLFW");

  // Register GLFW error callback
  glfwSetErrorCallback(glfw_error_callback);

  // Initialize GLFW memory allocator structure;
  // TODO: This is only available in conan 3.4, which conan does not support yet

  if (glfwInit() != GLFW_TRUE) {
    return false;
  }

  /*******************************
   *    Validating monitors      *
   *******************************/
  int monitor_count = 0;
  GLFWmonitor **monitors = glfwGetMonitors(&monitor_count);

  if (monitors == nullptr) {
    glfwTerminate();
    return false;
  }

  log_info("Monitors detected: {}", monitor_count);

  for (int i = 0; i < monitor_count; i++) {
    int width = 0, height = 0;
    float xscale = 0, yscale = 0;
    int xpos = 0, ypos = 0;
    int w_xpos = 0, w_ypos = 0, w_width = 0, w_height = 0;

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    glfwGetMonitorPhysicalSize(monitors[i], &width, &height);

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    glfwGetMonitorContentScale(monitors[i], &xscale, &yscale);

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    glfwGetMonitorPos(monitors[i], &xpos, &ypos);

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    glfwGetMonitorWorkarea(monitors[i], &w_xpos, &w_ypos, &w_width, &w_height);

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const char *name = glfwGetMonitorName(monitors[i]);

    if (name == nullptr) {
      log_error("Unnamed monitor {}", i);
    } else {
      // clang-format off
      log_info(
          "Properties of monitor {}:\n"
          "  Monitor name: {}.\n"
          "  Physical size (width, height): {}, {}.\n"
          "  Content scale (x, y): {}, {}.\n"
          "  Virtual position: (x, y): {}, {}.\n"
          "  Work area (x, y, width, height): {}, {}, {}, {}.",
          i,
          name,
          width,
          height,
          xscale,
          yscale,
          xpos,
          ypos,
          w_xpos,
          w_ypos,
          w_width,
          w_height
      );
      // clang-format on
    }
  }

  /*******************************
   *       Window Creation       *
   *******************************/
  log_info("Initializing engine window");

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

// TODO: Is this macro name correct?
#ifdef SURGE_SYSTEM_MacOSX
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  if (engine_config.windowed) {
    window.reset(glfwCreateWindow(gsl::narrow_cast<int>(engine_config.window_width),
                                  gsl::narrow_cast<int>(engine_config.window_height),
                                  engine_config.window_name, nullptr, nullptr));
  } else {
    window.reset(glfwCreateWindow(gsl::narrow_cast<int>(engine_config.window_width),
                                  gsl::narrow_cast<int>(engine_config.window_height),
                                  engine_config.window_name,

                                  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                                  monitors[engine_config.window_monitor_index < monitor_count
                                               ? engine_config.window_monitor_index
                                               : 0],
                                  nullptr));
  }

  if (window == nullptr) {
    glfwTerminate();
    return false;
  }

  log_info("Engine window created");

  /*******************************
   *       OpenGL Context        *
   ******************************/
  glfwMakeContextCurrent(window.get());
  glfwSwapInterval(1); // TODO: Set vsync via code;

  /*******************************
   *      Input callbacks        *
   *******************************/
  glfwSetKeyCallback(window.get(), glfw_key_callback);
  glfwSetMouseButtonCallback(window.get(), glfw_mouse_button_callback);
  glfwSetScrollCallback(window.get(), glfw_scroll_callback);

  /*******************************
   *             GLAD            *
   *******************************/
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
    log_error("Failed to initialize GLAD");
    window.reset();
    glfwTerminate();
    return false;
  }

  // Resize callback and viewport creation.
  glfwSetFramebufferSizeCallback(window.get(), framebuffer_size_callback);

  /*******************************
   *       OpenGL Options        *
   *******************************/
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  /*******************************
   *           SHADERS           *
   *******************************/
  if (!(std::filesystem::exists(engine_config.root_dir)
        && std::filesystem::is_directory(engine_config.root_dir))) {
    log_error(
        "The path {} in the configuration value \"engine_root_dir\" is not a valid directory.",
        engine_config.root_dir);

    window.reset();
    glfwTerminate();
    return false;
  }

  log_info("Compiling shaders");

  default_shaders::sprite_shader
      = create_program("shaders/sprite.vert", "shaders/sprite.frag").value_or(0);
  default_shaders::image_shader
      = create_program("shaders/image.vert", "shaders/image.frag").value_or(0);

  /*******************************
   *       VIEW/PROJECTION       *
   *******************************/
  const auto view_matrix{glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                                     glm::vec3(0.0f, 1.0f, 0.0f))};

  const auto projection_matrix{glm::ortho(0.0f, gsl::narrow_cast<float>(engine_config.window_width),
                                          gsl::narrow_cast<float>(engine_config.window_height),
                                          0.0f, 0.0f, 1.1f)};

  glUseProgram(default_shaders::sprite_shader);
  set_uniform(default_shaders::sprite_shader, "view", view_matrix);
  set_uniform(default_shaders::sprite_shader, "projection", projection_matrix);

  glUseProgram(default_shaders::image_shader);
  set_uniform(default_shaders::image_shader, "view", view_matrix);
  set_uniform(default_shaders::image_shader, "projection", projection_matrix);

  /*******************************
   *           CURSORS           *
   *******************************/
  if (engine_config.show_cursor) {
    glfwSetInputMode(window.get(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  } else {
    glfwSetInputMode(window.get(), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
  }

  return true;
}

void surge::engine_window::glfw_error_callback(int code, const char *description) noexcept {
  log_error("GLFW error code {}: {}", code, description);
}

void surge::engine_window::framebuffer_size_callback(GLFWwindow *, int width, int height) noexcept {
  glViewport(GLint{0}, GLint{0}, GLsizei{width}, GLsizei{height});
}