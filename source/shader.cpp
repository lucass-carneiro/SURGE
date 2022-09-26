#include "shader.hpp"

#include "file.hpp"
#include "options.hpp"

#include <filesystem>

#ifdef SURGE_SYSTEM_IS_POSIX
#  include <cerrno>
#  include <cstring>
#  include <fcntl.h>
#  include <sys/mman.h>
#endif

#ifdef SURGE_SYSTEM_IS_POSIX
auto surge::dynamic_shader::mmap_file() noexcept -> char * {

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  const auto fd = open(shader_path.c_str(), O_RDONLY);

  if (fd == -1) {
    glog<log_event::message>("Unable to open {}: {}", shader_path.c_str(), std::strerror(errno));
    return nullptr;
  }

  auto memory_map
      = mmap(nullptr, std::filesystem::file_size(shader_path), PROT_READ, MAP_SHARED, fd, 0);

  close(fd);

  if (memory_map == MAP_FAILED) {
    glog<log_event::error>("Error while memory mapping {}: {}", shader_path.c_str(),
                           std::strerror(errno));
    return nullptr;
  }

  return static_cast<char *>(memory_map);
}

void surge::dynamic_shader::munmap_file(char *file) noexcept {
  munmap(static_cast<void *>(file), std::filesystem::file_size(shader_path));
}
#endif

auto surge::dynamic_shader::load() noexcept -> char * {
  glog<log_event::message>("Loading shader file {}", shader_path.c_str());

  // Validate the shader file paths
  switch (shader_type) {

  case GL_VERTEX_SHADER:
    if (validate_path(shader_path, ".vert")) {
      return nullptr;
    }
    break;

  case GL_FRAGMENT_SHADER:
    if (validate_path(shader_path, ".frag")) {
      return nullptr;
    }
    break;

  default:
    glog<log_event::error>("Unrecognized shader type {}", shader_type);
    return nullptr;
  }

  return mmap_file();
}

void surge::dynamic_shader::compile() noexcept {
  glog<log_event::message>("Compiling shader \"{}\"", shader_name);

  // Load the shader
  const auto shader_src = load();

  if (shader_src == nullptr) {
    shader_handle = {};
    return;
  }

  // Create an empty shader handle
  GLuint shader_handle_tmp = glCreateShader(shader_type);

  // Send the shader source code to GL
  glShaderSource(shader_handle_tmp, 1, &shader_src, nullptr);

  // Compile the shader
  glCompileShader(shader_handle_tmp);

  munmap_file(shader_src);

  GLint is_compiled{0};
  glGetShaderiv(shader_handle_tmp, GL_COMPILE_STATUS, &is_compiled);

  if (is_compiled == GL_FALSE) {
    GLint max_length{0};
    glGetShaderiv(shader_handle_tmp, GL_INFO_LOG_LENGTH, &max_length);

    // The maxLength includes the NULL character
    constexpr const GLsizei info_buffer_size{512};
    GLsizei returnd_info_size{0};
    std::array<GLchar, info_buffer_size> info_log{};

    glGetShaderInfoLog(shader_handle_tmp, info_buffer_size, &returnd_info_size, info_log.data());

    // We don't need the shader anymore.
    glDeleteShader(shader_handle_tmp);

    glog<log_event::error>("Shader \"{}\" compilation failed:\n  {}", shader_name, info_log.data());

    shader_handle = {};
    return;
  }

  glog<log_event::message>("Shader \"{}\" compiled succesfully", shader_name);
  shader_handle = shader_handle_tmp;
}

void surge::static_shader::compile() noexcept {
  glog<log_event::message>("Compiling shader \"{}\"", shader_name);

  // Create an empty shader handle
  GLuint shader_handle_tmp = glCreateShader(shader_type);

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
    constexpr const GLsizei info_buffer_size{512};
    GLsizei returnd_info_size{0};
    std::array<GLchar, info_buffer_size> info_log{};

    glGetShaderInfoLog(shader_handle_tmp, info_buffer_size, &returnd_info_size, info_log.data());

    // We don't need the shader anymore.
    glDeleteShader(shader_handle_tmp);

    glog<log_event::error>("Shader \"{}\" compilation failed:\n  {}", shader_name, info_log.data());

    shader_handle = {};
  }

  glog<log_event::message>("Shader \"{}\" compiled succesfully", shader_name);
  shader_handle = shader_handle_tmp;
}