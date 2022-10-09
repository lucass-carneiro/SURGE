#ifndef SURGE_LINK_PROGRAM
#define SURGE_LINK_PROGRAM

#include "load_and_compile_from_path.hpp"
#include "load_and_compile_from_text.hpp"

namespace surge {

template <bool destroy_shaders = true> [[nodiscard]] inline auto
link_shader_program(GLuint vertex_shader_handle, GLuint fragment_shader_handle) noexcept
    -> std::optional<GLuint> {

  glog<log_event::message>("Linking shader handles {} and {}.", vertex_shader_handle,
                           fragment_shader_handle);

  // Get a program object.
  GLuint program_handle_tmp{glCreateProgram()};

  // Attach our shaders to our program
  glAttachShader(program_handle_tmp, vertex_shader_handle);
  glAttachShader(program_handle_tmp, fragment_shader_handle);

  // Link our program
  glLinkProgram(program_handle_tmp);

  GLint is_linked{0};
  glGetProgramiv(program_handle_tmp, GL_LINK_STATUS, &is_linked);

  if (is_linked == GL_FALSE) {
    GLint max_length = 0;
    glGetProgramiv(program_handle_tmp, GL_INFO_LOG_LENGTH, &max_length);

    // The max_length includes the NULL character
    GLsizei returnd_info_size{0};
    std::array<GLchar, SURGE_OPENGL_ERROR_BUFFER_SIZE> info_log{};

    glGetProgramInfoLog(program_handle_tmp, SURGE_OPENGL_ERROR_BUFFER_SIZE, &returnd_info_size,
                        info_log.data());

    // Cleanup
    glDetachShader(program_handle_tmp, vertex_shader_handle);
    glDetachShader(program_handle_tmp, fragment_shader_handle);
    glDeleteProgram(program_handle_tmp);

    glog<log_event::message>("Failed to link shader handles {} and {} to create program:\n  {}",
                             vertex_shader_handle, fragment_shader_handle, info_log.data());
    return {};

  } else {

    glDetachShader(program_handle_tmp, vertex_shader_handle);
    glDetachShader(program_handle_tmp, fragment_shader_handle);

    if constexpr (destroy_shaders) {
      glog<log_event::message>("Destroying shaders handles {} and {}", vertex_shader_handle,
                               fragment_shader_handle);
      glDeleteShader(vertex_shader_handle);
      glDeleteShader(fragment_shader_handle);
    }

    glog<log_event::message>("Shader handles {} and {} linked succesfully. Program handle: {}",
                             vertex_shader_handle, fragment_shader_handle, program_handle_tmp);

    return program_handle_tmp;
  }
}

} // namespace surge

#endif