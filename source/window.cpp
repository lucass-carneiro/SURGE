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
#include <fstream>
#include <iostream>
#include <sstream>
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

void surge::shader::compile() noexcept {

  log_all<log_event::message>("Compiling shader \"{}\"", name);

  // Create an empty shader handle
  GLuint shader_handle = glCreateShader(type);

  // Send the shader source code to GL
  glShaderSource(shader_handle, 1, &source, nullptr);

  // Compile the shader
  glCompileShader(shader_handle);

  GLint is_compiled{0};
  glGetShaderiv(shader_handle, GL_COMPILE_STATUS, &is_compiled);

  if (is_compiled == GL_FALSE) {
    GLint max_length{0};
    glGetShaderiv(shader_handle, GL_INFO_LOG_LENGTH, &max_length);

    // The maxLength includes the NULL character
    constexpr const GLsizei info_buffer_size{512};
    GLsizei returnd_info_size{0};
    std::array<GLchar, info_buffer_size> info_log{};

    glGetShaderInfoLog(shader_handle, info_buffer_size, &returnd_info_size,
                       info_log.data());

    // We don't need the shader anymore.
    glDeleteShader(shader_handle);

    log_all<log_event::error>("Shader \"{}\" compilation failed:\n  {}", name,
                              info_log.data());

    handle = {};

    return;
  }

  log_all<log_event::message>("Shader \"{}\" compiled succesfully", name);
  handle = shader_handle;
}

auto surge::link_shaders(shader &vertex_shader,
                         shader &fragment_shader) noexcept
    -> std::optional<GLuint> {

  log_all<log_event::message>("Linking shaders \"{}\" and \"{}\"",
                              vertex_shader.get_name(),
                              fragment_shader.get_name());

  // Get a program object.
  GLuint program{glCreateProgram()};

  // Attach our shaders to our program
  glAttachShader(program, vertex_shader.get_handle().value());
  glAttachShader(program, fragment_shader.get_handle().value());

  // Link our program
  glLinkProgram(program);

  GLint is_linked{0};
  glGetProgramiv(program, GL_LINK_STATUS, &is_linked);

  if (is_linked == GL_FALSE) {

    GLint max_length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &max_length);

    // The max_length includes the NULL character
    constexpr const GLsizei info_buffer_size{512};
    GLsizei returnd_info_size{0};
    std::array<GLchar, info_buffer_size> info_log{};

    glGetProgramInfoLog(program, info_buffer_size, &returnd_info_size,
                        info_log.data());

    // Cleanup
    glDeleteProgram(program);

    log_all<log_event::message>("Failed to link \"{}\" and \"{}\":\n  {}",
                                vertex_shader.get_name(),
                                fragment_shader.get_name(), info_log.data());
    return {};
  }

  glDetachShader(program, vertex_shader.get_handle().value());
  glDetachShader(program, fragment_shader.get_handle().value());

  log_all<log_event::message>("Shaders \"{}\" and \"{}\" linked succesfully",
                              vertex_shader.get_name(),
                              fragment_shader.get_name());

  return program;
}