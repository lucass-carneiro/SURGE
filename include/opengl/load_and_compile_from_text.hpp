#ifndef SURGE_LOAD_AND_COMPILE_FROM_TEXT_HPP
#define SURGE_LOAD_AND_COMPILE_FROM_TEXT_HPP

#include "headers.hpp"
#include "log.hpp"

#include <optional>

namespace surge {

template <GLenum shader_type> [[nodiscard]] inline auto
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
load_and_compile(const char *shader_source, const char *shader_name) noexcept
    -> std::optional<GLuint> {

  glog<log_event::message>("Compiling shader {}", shader_name);

  // Create an empty shader handle
  const GLuint shader_handle_tmp = glCreateShader(shader_type);

  // Send the shader source code to GL
  glShaderSource(shader_handle_tmp, 1, &shader_source, nullptr);

  // Compile the shader
  glCompileShader(shader_handle_tmp);

  GLint is_compiled{0};
  glGetShaderiv(shader_handle_tmp, GL_COMPILE_STATUS, &is_compiled);

  if (is_compiled == GL_FALSE) {
    GLint max_length{0};
    glGetShaderiv(shader_handle_tmp, GL_INFO_LOG_LENGTH, &max_length);

    // The maxLength includes the NULL character
    GLsizei returnd_info_size{0};
    std::array<GLchar, SURGE_OPENGL_ERROR_BUFFER_SIZE> info_log{};

    glGetShaderInfoLog(shader_handle_tmp, SURGE_OPENGL_ERROR_BUFFER_SIZE, &returnd_info_size,
                       info_log.data());

    // We don't need the shader anymore.
    glDeleteShader(shader_handle_tmp);

    glog<log_event::error>("Shader \"{}\" compilation failed:\n  {}", shader_name, info_log.data());

    return {};
  } else {
    glog<log_event::error>("Shader \"{}\" compilation succesfull", shader_name);
    return shader_handle_tmp;
  }
}

} // namespace surge

#endif // SURGE_LOAD_AND_COMPILE_FROM_TEXT_HPP