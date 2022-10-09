#include "window.hpp"

#include "log.hpp"
#include "options.hpp"
#include "safe_ops.hpp"

//clang-format off
#include <EASTL/set.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
//clang-format on

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

surge::global_engine_window::~global_engine_window() {
  glog<log_event::message>("Deleting window.");

  // Calling reset on the window* guarantees that it will be destroyed before
  // glfwTerminate
  window.reset();

  if (glfw_init_success) {
    glfwTerminate();
  }
}

auto surge::global_engine_window::init() noexcept -> bool {
  glog<log_event::message>("Initializing window");

  // Retrieve, parse and cast configuration values from config script of the main thread VM (the
  // last one in the array)
  auto L{global_lua_states::get().back().get()};
  const auto engine_config{get_lua_engine_config(L)};

  if (!engine_config) {
    glfw_init_success = false;
    return glfw_init_success;
  }

  window_width = engine_config->window_width;
  window_height = engine_config->window_height;
  window_name = engine_config->window_name;
  windowed = engine_config->windowed;
  window_monitor_index = engine_config->window_monitor_index;
  clear_color_r = engine_config->clear_color[0];
  clear_color_g = engine_config->clear_color[1];
  clear_color_b = engine_config->clear_color[2];
  clear_color_a = engine_config->clear_color[3];

  // Register GLFW callbacks
  glfwSetErrorCallback(surge::glfw_error_callback);

  // Initialize GLFW memory allocator structure;
  // TODO: This is only available in conan 3.4, which conan does not support yet

  // Initialize glfw
  if (glfwInit() != GLFW_TRUE) {
    glfw_init_success = false;
    return glfw_init_success;
  }

  // Validate monitor index
  auto monitors = querry_available_monitors();
  if (!monitors.has_value()) {
    glfwTerminate();
    glfw_init_success = false;
    return glfw_init_success;
  }

  if (window_monitor_index >= monitors.value().second) {
    glog<log_event::warning>("Unable to set window monitor to {} because there are only {} "
                             "monitors. Using default monitor index 0",
                             window_monitor_index, monitors.value().second);
    window_monitor_index = 0;
  }

  // GLFW window creation
  glog<log_event::message>("Initializing engine window");
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef SURGE_SYSTEM_MacOSX // TODO: Is this macro name correct?
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  if (windowed) {
    (void)window.release();
    window.reset(glfwCreateWindow(window_width, window_height, window_name, nullptr, nullptr));
  } else {
    (void)window.release();
    window.reset(glfwCreateWindow(window_width, window_height, window_name,
                                  (monitors.value().first)[window_monitor_index], nullptr));
  }

  if (window == nullptr) {
    glfwTerminate();
    glfw_init_success = false;
    return glfw_init_success;
  }

  // OpenGL context creation
  glfwMakeContextCurrent(window.get());
  glfwSwapInterval(1); // TODO: Set vsync via code;

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
    glog<log_event::error>("Failed to initialize GLAD");
    window.reset();
    glfwTerminate();
    glfw_init_success = false;
    return glfw_init_success;
  }

  // Resize callback and viewport creation.
  glfwSetFramebufferSizeCallback(window.get(), framebuffer_size_callback);

  glfw_init_success = true;
  return glfw_init_success;
}

auto surge::global_engine_window::querry_available_monitors() noexcept
    -> std::optional<std::pair<GLFWmonitor **, int>> {

  int count = 0;
  GLFWmonitor **monitors = glfwGetMonitors(&count);

  if (monitors == nullptr) {
    return {};
  }

  glog<log_event::message>("Monitors detected: {}", count);

  for (int i = 0; i < count; i++) {
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
      return {};
    }

    // clang-format off
    glog<log_event::message>(
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

  return std::make_pair(monitors, count);
}

void surge::glfw_error_callback(int code, const char *description) noexcept {
  glog<log_event::error>("GLFW error code {}: {}", code, description);
}

void surge::framebuffer_size_callback(GLFWwindow *, int width, int height) noexcept {
  glViewport(GLint{0}, GLint{0}, GLsizei{width}, GLsizei{height});
}