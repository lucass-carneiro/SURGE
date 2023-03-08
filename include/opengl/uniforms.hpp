#ifndef SURGE_GL_UNIFORMS_HPP
#define SURGE_GL_UNIFORMS_HPP

#include "glm.hpp"
#include "program.hpp"

#include <concepts>

namespace surge {

void set_uniform(GLuint program_handle, const char *uniform_name, bool value) noexcept;

void set_uniform(GLuint program_handle, const char *uniform_name, GLint value) noexcept;

void set_uniform(GLuint program_handle, const char *uniform_name, float value) noexcept;

void set_uniform(GLuint program_handle, const char *uniform_name, const glm::bvec2 &value) noexcept;

void set_uniform(GLuint program_handle, const char *uniform_name, const glm::ivec2 &value) noexcept;

void set_uniform(GLuint program_handle, const char *uniform_name, const glm::fvec2 &value) noexcept;

void set_uniform(GLuint program_handle, const char *uniform_name, const glm::bvec3 &value) noexcept;

void set_uniform(GLuint program_handle, const char *uniform_name, const glm::ivec3 &value) noexcept;

void set_uniform(GLuint program_handle, const char *uniform_name, const glm::fvec3 &value) noexcept;

void set_uniform(GLuint program_handle, const char *uniform_name, const glm::bvec4 &value) noexcept;

void set_uniform(GLuint program_handle, const char *uniform_name, const glm::ivec4 &value) noexcept;

void set_uniform(GLuint program_handle, const char *uniform_name, const glm::fvec4 &value) noexcept;

void set_uniform(GLuint program_handle, const char *uniform_name, const glm::mat4 &value) noexcept;

} // namespace surge

#endif // SURGE_GL_UNIFORMS_HPP