#include "window.hpp"
#include "arena_allocator.hpp"
#include "log.hpp"
#include "options.hpp"

//clang-format off
#include <EASTL/set.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
//clang-format on

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <tl/expected.hpp>

void surge::glfw_error_callback(int code, const char *description) noexcept {
  log_all<log_event::error>("GLFW error code {}: {}", code, description);
}

void surge::framebuffer_size_callback(GLFWwindow *, int width,
                                      int height) noexcept {
  glViewport(GLint{0}, GLint{0}, GLsizei{width}, GLsizei{height});
}

auto surge::querry_available_monitors() noexcept
    -> std::optional<std::pair<GLFWmonitor **, std::size_t>> {
  using tl::unexpected;

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