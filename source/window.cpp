#include "window.hpp"
#include "allocators.hpp"
#include "log.hpp"
#include "options.hpp"
#include "safe_ops.hpp"
#include "squirrel_bindings.hpp"

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
  global_stdout_log_manager::get().log<log_event::message>("Deleating window.");

  // Calling reset on the window* guarantees that it will be destroyed before
  // glfwTerminate
  window.reset();

  if (glfw_init_success) {
    glfwTerminate();
  }
}

auto surge::global_engine_window::init(surge_allocator &) noexcept -> bool {

  // Retrieve, parse and cast configuration values from config script
  const auto window_width_optional =
      global_squirrel_vm::get().surge_retrieve<SQInteger, int>(
          _SC("window_width"));

  const auto window_height_optional =
      global_squirrel_vm::get().surge_retrieve<SQInteger, int>(
          _SC("window_height"));

  const auto window_name_optional =
      global_squirrel_vm::get().surge_retrieve<const SQChar *>(
          _SC("window_name"));

  auto windowed_optional =
      global_squirrel_vm::get().surge_retrieve<SQBool>(_SC("windowed"));

  auto window_monitor_index_optional =
      global_squirrel_vm::get().surge_retrieve<SQInteger>(
          _SC("window_monitor_index"));

  auto clear_color_r_optional =
      global_squirrel_vm::get().surge_retrieve<SQFloat>(_SC("clear_color_r"));

  auto clear_color_g_optional =
      global_squirrel_vm::get().surge_retrieve<SQFloat>(_SC("clear_color_g"));

  auto clear_color_b_optional =
      global_squirrel_vm::get().surge_retrieve<SQFloat>(_SC("clear_color_b"));

  auto clear_color_a_optional =
      global_squirrel_vm::get().surge_retrieve<SQFloat>(_SC("clear_color_a"));

  bool parsed =
      window_width_optional.has_value() && window_height_optional.has_value() &&
      window_name_optional.has_value() && windowed_optional.has_value() &&
      window_monitor_index_optional.has_value() &&
      clear_color_r_optional.has_value() &&
      clear_color_g_optional.has_value() &&
      clear_color_b_optional.has_value() && clear_color_a_optional.has_value();

  if (!parsed) {
    glfw_init_success = false;
    return glfw_init_success;
  }

  window_width = window_width_optional.value();
  window_height = window_height_optional.value();
  window_name = window_name_optional.value();
  windowed = windowed_optional.value();
  window_monitor_index = window_monitor_index_optional.value();
  clear_color_r = clear_color_r_optional.value();
  clear_color_g = clear_color_g_optional.value();
  clear_color_b = clear_color_b_optional.value();
  clear_color_a = clear_color_a_optional.value();

  // Register GLFW callbacks
  glfwSetErrorCallback(surge::glfw_error_callback);

  // Initialize GLFW memory allocator structure;
  /* TODO: This is only available in conan 3.4, which conan does not support yet
  GLFWallocator glfw_allocator;
  glfw_allocator.allocate = glfw_allocate;
  glfw_allocator.reallocate = glfw_reallocate;
  glfw_allocator.deallocate = glfw_free;
  */

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
    log_all<log_event::warning>(
        "Unable to set window monitor to {} because there are only {} "
        "monitors. Using default monitor index 0",
        window_monitor_index, monitors.value().second);
    window_monitor_index = 0;
  }

  // GLFW window creation
  log_all<log_event::message>("Initializing engine window");
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef SURGE_SYSTEM_MacOSX // TODO: Is this macro name correct?
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  if (windowed == SQBool{true}) {
    (void)window.release();
    window.reset(glfwCreateWindow(window_width, window_height, window_name,
                                  nullptr, nullptr));
  } else {
    (void)window.release();
    window.reset(glfwCreateWindow(
        window_width, window_height, window_name,
        (monitors.value().first)[window_monitor_index], nullptr));
  }

  if (window == nullptr) {
    glfwTerminate();
    glfw_init_success = false;
    return glfw_init_success;
  }

  // OpenGL context creation
  glfwMakeContextCurrent(window.get());

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
    log_all<log_event::error>("Failed to initialize GLAD");
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

  log_all<log_event::message>("Monitors detected: {}", count);

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
    log_all<log_event::message>(
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
  log_all<log_event::error>("GLFW error code {}: {}", code, description);
}

void surge::framebuffer_size_callback(GLFWwindow *, int width,
                                      int height) noexcept {
  glViewport(GLint{0}, GLint{0}, GLsizei{width}, GLsizei{height});
}

// TODO: This is not available in conan yer. Also, revise the casts
auto surge::glfw_allocate(std::size_t size, void *user) noexcept -> void * {
  surge_allocator *allocator{static_cast<surge_allocator *>(user)};
  return allocator->malloc(size);
}

auto surge::glfw_reallocate(void *block, std::size_t size, void *user) noexcept
    -> void * {
  surge_allocator *allocator{static_cast<surge_allocator *>(user)};
  return allocator->realloc(block, size);
}

auto surge::glfw_free(void *block, void *user) noexcept {
  surge_allocator *allocator{static_cast<surge_allocator *>(user)};
  allocator->free(block);
}