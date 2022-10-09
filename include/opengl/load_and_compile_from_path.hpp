#ifndef SURGE_LOAD_AND_COMPILE_FROM_PATH_HPP
#define SURGE_LOAD_AND_COMPILE_FROM_PATH_HPP

#include "file.hpp"
#include "headers.hpp"
#include "log.hpp"

namespace surge {

template <GLenum shader_type, surge_allocator alloc_t> [[nodiscard]] inline auto
load_and_compile(alloc_t *allocator, const std::filesystem::path &p) noexcept
    -> std::optional<GLuint> {

  glog<log_event::message>("Loading shader file {}", p.c_str());

  load_file_return_t file{};

  if constexpr (shader_type == GL_VERTEX_SHADER) {
    file = load_file<alloc_t, true>(allocator, p, ".vert");

  } else if constexpr (shader_type == GL_FRAGMENT_SHADER) {
    file = load_file<alloc_t, true>(allocator, p, ".frag");

  } else {
    glog<log_event::error>("Unrecognized shader type {}", shader_type);
    return {};
  }

  if (!file) {
    glog<log_event::error>("Unable to load shader file {}", p.c_str());
    return {};
  }

  // Get the source code in GL format
  auto file_source{static_cast<const GLchar *>(static_cast<void *>(file.value().data()))};

  // Create an empty shader handle
  const GLuint shader_handle_tmp = glCreateShader(shader_type);

  // Send the shader source code to GL
  glShaderSource(shader_handle_tmp, 1, &file_source, nullptr);

  // Compile the shader
  glCompileShader(shader_handle_tmp);

  // The file is no longer needed.
  allocator->free(static_cast<void *>(file.value().data()));

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

    glog<log_event::error>("Shader \"{}\" handle {} compilation failed:\n  {}", p.c_str(),
                           shader_handle_tmp, info_log.data());

    return {};
  } else {
    glog<log_event::message>("Shader \"{}\" handle {} compilation succesfull", p.c_str(),
                             shader_handle_tmp);
    return shader_handle_tmp;
  }
}

} // namespace surge

#endif // SURGE_LOAD_AND_COMPILE_FROM_PATH_HPP