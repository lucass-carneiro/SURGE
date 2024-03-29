#include "options.hpp"
#include "renderer.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <gsl/gsl-lite.hpp>

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#endif

void surge::renderer::uniforms::set(GLuint program_handle, const char *uniform_name,
                                    bool value) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::renderer::unifors::set(bool)");
#endif

  glUniform1i(glGetUniformLocation(program_handle, uniform_name), static_cast<GLboolean>(value));
}

void surge::renderer::uniforms::set(GLuint program_handle, const char *uniform_name,
                                    GLint value) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::renderer::unifors::set(GLint)");
#endif

  glUniform1i(glGetUniformLocation(program_handle, uniform_name), value);
}

void surge::renderer::uniforms::set(GLuint program_handle, const char *uniform_name,
                                    GLuint64 value) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::renderer::unifors::set(GLuint64)");
#endif
  glUniformHandleui64ARB(glGetUniformLocation(program_handle, uniform_name), value);
}

void surge::renderer::uniforms::set(GLuint program_handle, const char *uniform_name,
                                    float value) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::renderer::unifors::set(float)");
#endif

  glUniform1f(glGetUniformLocation(program_handle, uniform_name), value);
}

void surge::renderer::uniforms::set(GLuint program_handle, const char *uniform_name,
                                    const glm::bvec2 &value) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::renderer::unifors::set(glm::bvec2)");
#endif

  glUniform2i(glGetUniformLocation(program_handle, uniform_name), static_cast<GLint>(value[0]),
              static_cast<GLint>(value[1]));
}

void surge::renderer::uniforms::set(GLuint program_handle, const char *uniform_name,
                                    const glm::ivec2 &value) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::renderer::unifors::set(glm::ivec2)");
#endif

  glUniform2i(glGetUniformLocation(program_handle, uniform_name), value[0], value[1]);
}

void surge::renderer::uniforms::set(GLuint program_handle, const char *uniform_name,
                                    const glm::fvec2 &value) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::renderer::unifors::set(glm::fvec2)");
#endif

  glUniform2f(glGetUniformLocation(program_handle, uniform_name), value[0], value[1]);
}

void surge::renderer::uniforms::set(GLuint program_handle, const char *uniform_name,
                                    const glm::bvec3 &value) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::renderer::unifors::set(glm::bvec3)");
#endif
  glUniform3i(glGetUniformLocation(program_handle, uniform_name), static_cast<GLint>(value[0]),
              static_cast<GLint>(value[1]), static_cast<GLint>(value[2]));
}

void surge::renderer::uniforms::set(GLuint program_handle, const char *uniform_name,
                                    const glm::ivec3 &value) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::renderer::unifors::set(glm::ivec3)");
#endif

  glUniform3i(glGetUniformLocation(program_handle, uniform_name), value[0], value[1], value[2]);
}

void surge::renderer::uniforms::set(GLuint program_handle, const char *uniform_name,
                                    const glm::fvec3 &value) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::renderer::unifors::set(glm::fvec3)");
#endif

  glUniform3f(glGetUniformLocation(program_handle, uniform_name), value[0], value[1], value[2]);
}

void surge::renderer::uniforms::set(GLuint program_handle, const char *uniform_name,
                                    const glm::bvec4 &value) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::renderer::unifors::set(glm::bvec4)");
#endif

  glUniform4i(glGetUniformLocation(program_handle, uniform_name), static_cast<GLint>(value[0]),
              static_cast<GLint>(value[1]), static_cast<GLint>(value[2]),
              static_cast<GLint>(value[3]));
}

void surge::renderer::uniforms::set(GLuint program_handle, const char *uniform_name,
                                    const glm::ivec4 &value) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::renderer::unifors::set(glm::ivec4)");
#endif

  glUniform4i(glGetUniformLocation(program_handle, uniform_name), value[0], value[1], value[2],
              value[3]);
}

void surge::renderer::uniforms::set(GLuint program_handle, const char *uniform_name,
                                    const glm::fvec4 &value) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::renderer::unifors::set(glm::fvec4)");
#endif

  glUniform4f(glGetUniformLocation(program_handle, uniform_name), value[0], value[1], value[2],
              value[3]);
}

void surge::renderer::uniforms::set(GLuint program_handle, const char *uniform_name,
                                    const glm::mat3 &value) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::renderer::unifors::set(glm::mat4)");
#endif

  glUniformMatrix3fv(glGetUniformLocation(program_handle, uniform_name), 1, GL_FALSE,
                     glm::value_ptr(value));
}

void surge::renderer::uniforms::set(GLuint program_handle, const char *uniform_name,
                                    const glm::mat4 &value) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::renderer::unifors::set(glm::mat4)");
#endif

  glUniformMatrix4fv(glGetUniformLocation(program_handle, uniform_name), 1, GL_FALSE,
                     glm::value_ptr(value));
}

void surge::renderer::uniforms::set(GLuint program_handle, const char *uniform_name,
                                    const float *values, usize size) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::renderer::unifors::set(float array)");
#endif
  glUniform1fv(glGetUniformLocation(program_handle, uniform_name), gsl::narrow_cast<GLsizei>(size),
               values);
}

void surge::renderer::uniforms::set(GLuint program_handle, const char *uniform_name,
                                    const GLuint64 *values, usize size) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::renderer::unifors::set(GLuint64 array)");
#endif
  glUniformHandleui64vARB(glGetUniformLocation(program_handle, uniform_name),
                          gsl::narrow_cast<GLsizei>(size), values);
}

void surge::renderer::uniforms::set(GLuint program_handle, const char *uniform_name,
                                    const glm::fvec3 *values, usize size) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::renderer::unifors::set(glm::fvec3 array)");
#endif
  glUniform3fv(glGetUniformLocation(program_handle, uniform_name), gsl::narrow_cast<GLsizei>(size),
               glm::value_ptr(values[0]));
}

void surge::renderer::uniforms::set(GLuint program_handle, const char *uniform_name,
                                    const glm::fvec4 *values, usize size) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::renderer::unifors::set(glm::fvec4 array)");
#endif
  glUniform4fv(glGetUniformLocation(program_handle, uniform_name), gsl::narrow_cast<GLsizei>(size),
               glm::value_ptr(values[0]));
}

void surge::renderer::uniforms::set(GLuint program_handle, const char *uniform_name,
                                    const glm::mat4 *values, usize size) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::renderer::unifors::set(glm::mat4 array)");
#endif
  glUniformMatrix4fv(glGetUniformLocation(program_handle, uniform_name),
                     gsl::narrow_cast<GLsizei>(size), GL_FALSE, glm::value_ptr(values[0]));
}