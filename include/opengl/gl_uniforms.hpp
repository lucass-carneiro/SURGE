#ifndef SURGE_GL_UNIFORMS_HPP
#define SURGE_GL_UNIFORMS_HPP

#include "create_program.hpp"
#include "glm.hpp"

#include <concepts>

namespace surge {

// clang-format off
template <typename T>
concept gl_scalar_uniform = 
  std::is_same<T, bool>::value    || 
  std::is_same<T, GLint>::value   ||
  std::is_same<T,GLfloat>::value;
// clang-format on

// clang-format off
template <glm::length_t N, typename T>
concept gl_nd_uniform =
  (N == 2 || N == 3 || N == 4)                                   &&
  (
    std::is_same<T, glm::vec<N, bool, glm::defaultp>>::value     ||
    std::is_same<T, glm::vec<N, GLint, glm::defaultp>>::value    ||
    std::is_same<T, glm::vec<N, GLfloat, glm::defaultp>>::value
  );
// clang-format on

template <gl_scalar_uniform T>
void set_uniform(GLuint program_handle, const char *uniform_name, T value) noexcept {
  if constexpr (std::is_same<T, bool>::value) {
    glUniform1i(glGetUniformLocation(program_handle, uniform_name), static_cast<GLint>(value));

  } else if constexpr (std::is_same<T, GLint>::value) {
    glUniform1i(glGetUniformLocation(program_handle, uniform_name), value);

  } else if constexpr (std::is_same<T, GLfloat>::value) {
    glUniform1f(glGetUniformLocation(program_handle, uniform_name), value);
  }
}

template <glm::length_t N, typename T>
  requires gl_nd_uniform<N, T>
void set_uniform(GLuint program_handle, const char *uniform_name,
                 glm::vec<N, T, glm::defaultp> value) noexcept {
  if constexpr (N == 2) {
    if constexpr (std::is_same<T, bool>::value) {
      glUniform2i(glGetUniformLocation(program_handle, uniform_name), static_cast<GLint>(value.x),
                  static_cast<GLint>(value.y));

    } else if constexpr (std::is_same<T, GLint>::value) {
      glUniform2i(glGetUniformLocation(program_handle, uniform_name), value.x, value.y);

    } else if constexpr (std::is_same<T, GLfloat>::value) {
      glUniform2f(glGetUniformLocation(program_handle, uniform_name), value.x, value.y);
    }
  } else if constexpr (N == 3) {
    if constexpr (std::is_same<T, bool>::value) {
      glUniform3i(glGetUniformLocation(program_handle, uniform_name), static_cast<GLint>(value.x),
                  static_cast<GLint>(value.y), static_cast<GLint>(value.z));

    } else if constexpr (std::is_same<T, GLint>::value) {
      glUniform3i(glGetUniformLocation(program_handle, uniform_name), value.x, value.y, value.z);

    } else if constexpr (std::is_same<T, GLfloat>::value) {
      glUniform3f(glGetUniformLocation(program_handle, uniform_name), value.x, value.y, value.z);
    }
  } else if constexpr (N == 4) {
    if constexpr (std::is_same<T, bool>::value) {
      glUniform4i(glGetUniformLocation(program_handle, uniform_name), static_cast<GLint>(value.x),
                  static_cast<GLint>(value.y), static_cast<GLint>(value.z),
                  static_cast<GLint>(value.w));

    } else if constexpr (std::is_same<T, GLint>::value) {
      glUniform4i(glGetUniformLocation(program_handle, uniform_name), value.x, value.y, value.z,
                  value.w);

    } else if constexpr (std::is_same<T, GLfloat>::value) {
      glUniform4f(glGetUniformLocation(program_handle, uniform_name), value.x, value.y, value.z,
                  value.w);
    }
  }
}

inline void set_uniform(GLuint program_handle, const char *uniform_name,
                        const glm::mat4 &value) noexcept {
  glUniformMatrix4fv(glGetUniformLocation(program_handle, uniform_name), 1, GL_FALSE,
                     glm::value_ptr(value));
}

} // namespace surge

#endif // SURGE_GL_UNIFORMS_HPP