#ifndef SURGE_RENDERER_HPP
#define SURGE_RENDERER_HPP

#include "config.hpp"
#include "error_types.hpp"
#include "gl_includes.hpp"
#include "integer_types.hpp"

#include <glm/glm.hpp>

namespace surge::renderer {

enum class capability : GLint { depth_test = GL_DEPTH_TEST, blend = GL_BLEND, wireframe };

enum class blend_src : GLint { alpha = GL_SRC_ALPHA };

enum class blend_dest : GLint { one_minus_src_alpha = GL_ONE_MINUS_SRC_ALPHA };

enum texture_filtering : GLint { nearest, linear, anisotropic };

enum class texture_wrap : GLint {
  repeat = GL_REPEAT,
  mirrored_repeat = GL_MIRRORED_REPEAT,
  clamp_to_edge = GL_CLAMP_TO_EDGE,
  clamp_to_border = GL_CLAMP_TO_BORDER
};

void enable(const capability cap) noexcept;
void disable(const capability cap) noexcept;

void blend_function(const blend_src src, const blend_dest dest) noexcept;

void clear(const config::clear_color &ccl) noexcept;

auto create_shader_program(const char *vertex_shader_path,
                           const char *fragment_shader_path) noexcept
    -> tl::expected<GLuint, error>;

auto create_compute_shader(const char *compute_shader_path) noexcept -> tl::expected<GLuint, error>;

void destroy_shader_program(GLuint program) noexcept;

void dispatch_compute(GLuint program, GLuint x, GLuint y, GLuint z) noexcept;
void dispatch_compute(GLuint program, GLuint x, GLuint y, GLuint z, GLbitfield barriers) noexcept;

namespace uniforms {

void set(GLuint program_handle, const char *uniform_name, bool value) noexcept;
void set(GLuint program_handle, const char *uniform_name, GLint value) noexcept;
void set(GLuint program_handle, const char *uniform_name, GLuint64 value) noexcept;
void set(GLuint program_handle, const char *uniform_name, float value) noexcept;
void set(GLuint program_handle, const char *uniform_name, const glm::bvec2 &value) noexcept;
void set(GLuint program_handle, const char *uniform_name, const glm::ivec2 &value) noexcept;
void set(GLuint program_handle, const char *uniform_name, const glm::fvec2 &value) noexcept;
void set(GLuint program_handle, const char *uniform_name, const glm::bvec3 &value) noexcept;
void set(GLuint program_handle, const char *uniform_name, const glm::ivec3 &value) noexcept;
void set(GLuint program_handle, const char *uniform_name, const glm::fvec3 &value) noexcept;
void set(GLuint program_handle, const char *uniform_name, const glm::bvec4 &value) noexcept;
void set(GLuint program_handle, const char *uniform_name, const glm::ivec4 &value) noexcept;
void set(GLuint program_handle, const char *uniform_name, const glm::fvec4 &value) noexcept;
void set(GLuint program_handle, const char *uniform_name, const glm::mat3 &value) noexcept;
void set(GLuint program_handle, const char *uniform_name, const glm::mat4 &value) noexcept;

void set(GLuint program_handle, const char *uniform_name, const GLuint64 *values,
         usize size) noexcept;
void set(GLuint program_handle, const char *uniform_name, const float *values, usize size) noexcept;
void set(GLuint program_handle, const char *uniform_name, const glm::fvec3 *values,
         usize size) noexcept;
void set(GLuint program_handle, const char *uniform_name, const glm::fvec4 *values,
         usize size) noexcept;
void set(GLuint program_handle, const char *uniform_name, const glm::mat4 *values,
         usize size) noexcept;

} // namespace uniforms

} // namespace surge::renderer

#endif // SURGE_RENDERER_HPP