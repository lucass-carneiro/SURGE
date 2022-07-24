#include "shader.hpp"

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