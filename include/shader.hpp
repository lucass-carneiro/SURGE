#ifndef SURGE_SHADER_HPP
#define SURGE_SHADER_HPP

#include "log.hpp"
#include "window.hpp"

#include <GL/gl.h>
#include <filesystem>
#include <memory>
#include <optional>
#include <tl/expected.hpp>
#include <type_traits>

namespace surge {

// loads shaders from file at runtime
class dynamic_shader {
public:
  dynamic_shader(std::filesystem::path path, GLenum type, const char *name = "Default shader name")
      : shader_path{std::move(path)}, shader_type{type}, shader_name{name} {
    compile();
  }

  [[nodiscard]] auto is_compiled() const noexcept -> bool { return shader_handle.has_value(); }

  [[nodiscard]] auto get_path() const noexcept -> const std::filesystem::path & {
    return shader_path;
  }

  [[nodiscard]] auto get_type() const noexcept -> GLenum { return shader_type; }

  [[nodiscard]] auto get_name() const noexcept -> const char * { return shader_name; }

  [[nodiscard]] auto get_handle() const noexcept -> std::optional<GLuint> { return shader_handle; }

  inline void destroy() noexcept {
    glog<log_event::message>("Deleting shader {}.", shader_name);

    if (shader_handle.has_value()) {
      glDeleteShader(shader_handle.value());
      shader_handle = {};
    }
  }

private:
  const std::filesystem::path shader_path;
  const GLenum shader_type;
  const char *shader_name;

  std::optional<GLuint> shader_handle;

  [[nodiscard]] auto mmap_file() noexcept -> char *;
  void munmap_file(char *file) noexcept;

  auto load() noexcept -> char *;

  void compile() noexcept;
};

// loads shaders at compile time
class static_shader {
public:
  static_shader(GLenum type, const char *source, const char *name = "Default shader name")
      : shader_type{type}, shader_source{source}, shader_name{name} {
    compile();
  }

  [[nodiscard]] auto is_compiled() const noexcept -> bool { return shader_handle.has_value(); }

  [[nodiscard]] auto get_type() const noexcept -> GLenum { return shader_type; }

  [[nodiscard]] auto get_source() const noexcept -> const char * { return shader_source; }

  [[nodiscard]] auto get_name() const noexcept -> const char * { return shader_name; }

  [[nodiscard]] auto get_handle() const noexcept -> std::optional<GLuint> { return shader_handle; }

  inline void destroy() noexcept {
    glog<log_event::message>("Deleting shader {}.", shader_name);

    if (shader_handle.has_value()) {
      glDeleteShader(shader_handle.value());
      shader_handle = {};
    }
  }

private:
  const GLenum shader_type;
  const char *shader_source;
  const char *shader_name;

  std::optional<GLuint> shader_handle;

  void compile() noexcept;
};

template <typename T> struct opengl_vec2 {
  T x;
  T y;

  opengl_vec2(T _x, T _y) : x{_x}, y{_y} {}
};

template <typename T> struct opengl_vec3 {
  T x;
  T y;
  T z;

  opengl_vec3(T _x, T _y, T _z) : x{_x}, y{_y}, z{_z} {}
  opengl_vec3(const opengl_vec2<T> &v2, T _z) : x{v2.x}, y{v2.y}, z{_z} {}
};

template <typename T> struct opengl_vec4 {
  T x;
  T y;
  T z;
  T w;

  opengl_vec4(T _x, T _y, T _z, T _w) : x{_x}, y{_y}, z{_z}, w{_w} {}
  opengl_vec4(const opengl_vec3<T> &v3, T _w) : x{v3.x}, y{v3.y}, z{v3.z}, w{_w} {}
  opengl_vec4(const opengl_vec2<T> &v2, T _z, T _w) : x{v2.x}, y{v2.y}, z{_z}, w{_w} {}
};

template <typename T>
concept opengl_uniform_type_single
    = std::is_same<T, bool>::value || std::is_same<T, int>::value || std::is_same<T, float>::value;

template <typename T>
concept opengl_shader
    = std::is_same<T, dynamic_shader>::value || std::is_same<T, static_shader>::value;

class shader_program {
public:
  template <opengl_shader shader_t>
  shader_program(shader_t &vertex_shader, shader_t &fragment_shader,
                 const char *name = "Default program name", bool destroy_shaders = true)
      : program_name{name} {

    glog<log_event::message>("Linking shaders \"{}\" and \"{}\" to form shader program \"{}\"",
                             vertex_shader.get_name(), fragment_shader.get_name(), program_name);

    // Get a program object.
    GLuint program_handle_tmp{glCreateProgram()};

    // Attach our shaders to our program
    glAttachShader(program_handle_tmp, vertex_shader.get_handle().value());
    glAttachShader(program_handle_tmp, fragment_shader.get_handle().value());

    // Link our program
    glLinkProgram(program_handle_tmp);

    GLint is_linked{0};
    glGetProgramiv(program_handle_tmp, GL_LINK_STATUS, &is_linked);

    if (is_linked == GL_FALSE) {

      GLint max_length = 0;
      glGetProgramiv(program_handle_tmp, GL_INFO_LOG_LENGTH, &max_length);

      // The max_length includes the NULL character
      constexpr const GLsizei info_buffer_size{512};
      GLsizei returnd_info_size{0};
      std::array<GLchar, info_buffer_size> info_log{};

      glGetProgramInfoLog(program_handle_tmp, info_buffer_size, &returnd_info_size,
                          info_log.data());

      // Cleanup
      glDetachShader(program_handle_tmp, vertex_shader.get_handle().value());
      glDetachShader(program_handle_tmp, fragment_shader.get_handle().value());
      glDeleteProgram(program_handle_tmp);

      glog<log_event::message>("Failed to link \"{}\" and \"{}\" to create program {}:\n  {}",
                               vertex_shader.get_name(), fragment_shader.get_name(), program_name,
                               info_log.data());
      program_handle = {};
      return;
    }

    glDetachShader(program_handle_tmp, vertex_shader.get_handle().value());
    glDetachShader(program_handle_tmp, fragment_shader.get_handle().value());

    if (destroy_shaders) {
      glog<log_event::message>("Program \"{}\" will delete shaders \"{}\" and \"{}\"", program_name,
                               vertex_shader.get_name(), fragment_shader.get_name());
      vertex_shader.destroy();
      fragment_shader.destroy();
    }

    glog<log_event::message>("Shaders \"{}\" and \"{}\" linked succesfully",
                             vertex_shader.get_name(), fragment_shader.get_name());

    program_handle = program_handle_tmp;
  }

  template <opengl_uniform_type_single uniform_type>
  void set_uniform(const char *uniform_name, uniform_type value) {

    if (!is_linked()) {
      glog<log_event::warning>("Cannot set uniform {} with value {} in {} "
                               "because the program is not linked",
                               uniform_name, value, program_name);
      return;
    }

    if constexpr (std::is_same<uniform_type, bool>::value) {
      glUniform1i(glGetUniformLocation(program_handle.value(), uniform_name),
                  static_cast<bool>(value));

    } else if constexpr (std::is_same<uniform_type, int>::value) {
      glUniform1i(glGetUniformLocation(program_handle.value(), uniform_name), value);

    } else if constexpr (std::is_same<uniform_type, float>::value) {
      glUniform1f(glGetUniformLocation(program_handle.value(), uniform_name), value);
    }
  }

  template <opengl_uniform_type_single uniform_type>
  void set_uniform(const char *uniform_name, const opengl_vec2<uniform_type> &value) {
    if (!is_linked()) {
      glog<log_event::warning>("Cannot set uniform {} with value {} in {} "
                               "because the program is not linked",
                               uniform_name, value, program_name);
      return;
    }

    if constexpr (std::is_same<uniform_type, bool>::value) {
      glUniform2i(glGetUniformLocation(program_handle.value(), uniform_name),
                  static_cast<bool>(value.x), static_cast<bool>(value.y));

    } else if constexpr (std::is_same<uniform_type, int>::value) {
      glUniform2i(glGetUniformLocation(program_handle.value(), uniform_name), value.x, value.y);

    } else if constexpr (std::is_same<uniform_type, float>::value) {
      glUniform2f(glGetUniformLocation(program_handle.value(), uniform_name), value.x, value.y);
    }
  }

  template <opengl_uniform_type_single uniform_type>
  void set_uniform(const char *uniform_name, const opengl_vec3<uniform_type> &value) {
    if (!is_linked()) {
      glog<log_event::warning>("Cannot set uniform {} with value {} in {} "
                               "because the program is not linked",
                               uniform_name, value, program_name);
      return;
    }

    if constexpr (std::is_same<uniform_type, bool>::value) {
      glUniform3i(glGetUniformLocation(program_handle.value(), uniform_name),
                  static_cast<bool>(value.x), static_cast<bool>(value.y),
                  static_cast<bool>(value.z));

    } else if constexpr (std::is_same<uniform_type, int>::value) {
      glUniform3i(glGetUniformLocation(program_handle.value(), uniform_name), value.x, value.y,
                  value.z);

    } else if constexpr (std::is_same<uniform_type, float>::value) {
      glUniform3f(glGetUniformLocation(program_handle.value(), uniform_name), value.x, value.y,
                  value.z);
    }
  }

  template <opengl_uniform_type_single uniform_type>
  void set_uniform(const char *uniform_name, const opengl_vec4<uniform_type> &value) {
    if (!is_linked()) {
      glog<log_event::warning>("Cannot set uniform {} with value {} in {} "
                               "because the program is not linked",
                               uniform_name, value, program_name);
      return;
    }

    if constexpr (std::is_same<uniform_type, bool>::value) {
      glUniform4i(glGetUniformLocation(program_handle.value(), uniform_name),
                  static_cast<bool>(value.x), static_cast<bool>(value.y),
                  static_cast<bool>(value.z), static_cast<bool>(value.w));

    } else if constexpr (std::is_same<uniform_type, int>::value) {
      glUniform4i(glGetUniformLocation(program_handle.value(), uniform_name), value.x, value.y,
                  value.z, value.w);

    } else if constexpr (std::is_same<uniform_type, float>::value) {
      glUniform4f(glGetUniformLocation(program_handle.value(), uniform_name), value.x, value.y,
                  value.z),
          value.w;
    }
  }

  [[nodiscard]] auto get_name() const noexcept -> const char * { return program_name; }

  [[nodiscard]] auto is_linked() const noexcept -> bool { return program_handle.has_value(); }

  void use() { glUseProgram(program_handle.value()); }

  void destroy() {
    glog<log_event::message>("Deleting program \"{}\".", program_name);

    if (program_handle.has_value()) {
      glDeleteProgram(program_handle.value());
      program_handle = {};
    }
  }

private:
  const char *program_name;
  std::optional<GLuint> program_handle;
};

} // namespace surge

#endif // SURGE_SHADER_HPP