#include "opengl/uniforms.hpp"

void surge::set_uniform(GLuint program_handle, const char *uniform_name, bool value) noexcept {
  glUniform1i(glGetUniformLocation(program_handle, uniform_name), static_cast<GLint>(value));
}

void surge::set_uniform(GLuint program_handle, const char *uniform_name, GLint value) noexcept {
  glUniform1i(glGetUniformLocation(program_handle, uniform_name), value);
}

void surge::set_uniform(GLuint program_handle, const char *uniform_name, float value) noexcept {
  glUniform1f(glGetUniformLocation(program_handle, uniform_name), value);
}

void surge::set_uniform(GLuint program_handle, const char *uniform_name,
                        const glm::bvec2 &value) noexcept {
  glUniform2i(glGetUniformLocation(program_handle, uniform_name), static_cast<GLint>(value[0]),
              static_cast<GLint>(value[1]));
}

void surge::set_uniform(GLuint program_handle, const char *uniform_name,
                        const glm::ivec2 &value) noexcept {
  glUniform2i(glGetUniformLocation(program_handle, uniform_name), value[0], value[1]);
}

void surge::set_uniform(GLuint program_handle, const char *uniform_name,
                        const glm::fvec2 &value) noexcept {
  glUniform2f(glGetUniformLocation(program_handle, uniform_name), value[0], value[1]);
}

void surge::set_uniform(GLuint program_handle, const char *uniform_name,
                        const glm::bvec3 &value) noexcept {
  glUniform3i(glGetUniformLocation(program_handle, uniform_name), static_cast<GLint>(value[0]),
              static_cast<GLint>(value[1]), static_cast<GLint>(value[2]));
}

void surge::set_uniform(GLuint program_handle, const char *uniform_name,
                        const glm::ivec3 &value) noexcept {
  glUniform3i(glGetUniformLocation(program_handle, uniform_name), value[0], value[1], value[2]);
}

void surge::set_uniform(GLuint program_handle, const char *uniform_name,
                        const glm::fvec3 &value) noexcept {
  glUniform3f(glGetUniformLocation(program_handle, uniform_name), value[0], value[1], value[2]);
}

void surge::set_uniform(GLuint program_handle, const char *uniform_name,
                        const glm::bvec4 &value) noexcept {
  glUniform4i(glGetUniformLocation(program_handle, uniform_name), static_cast<GLint>(value[0]),
              static_cast<GLint>(value[1]), static_cast<GLint>(value[2]),
              static_cast<GLint>(value[3]));
}

void surge::set_uniform(GLuint program_handle, const char *uniform_name,
                        const glm::ivec4 &value) noexcept {
  glUniform4i(glGetUniformLocation(program_handle, uniform_name), value[0], value[1], value[2],
              value[3]);
}

void surge::set_uniform(GLuint program_handle, const char *uniform_name,
                        const glm::fvec4 &value) noexcept {
  glUniform4f(glGetUniformLocation(program_handle, uniform_name), value[0], value[1], value[2],
              value[3]);
}

void surge::set_uniform(GLuint program_handle, const char *uniform_name,
                        const glm::mat4 &value) noexcept {
  glUniformMatrix4fv(glGetUniformLocation(program_handle, uniform_name), 1, GL_FALSE,
                     glm::value_ptr(value));
}