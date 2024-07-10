#ifndef SURGE_CORE_GL_ATOM_SHADERS_HPP
#define SURGE_CORE_GL_ATOM_SHADERS_HPP

#include "error_types.hpp"
#include "renderer_gl.hpp"

#include <tl/expected.hpp>

namespace surge::gl_atom::shader {

auto create_shader_program(const char *vertex_shader_path,
                           const char *fragment_shader_path) noexcept
    -> tl::expected<GLuint, error>;

auto create_compute_shader(const char *compute_shader_path) noexcept -> tl::expected<GLuint, error>;

void destroy_shader_program(GLuint program) noexcept;

void dispatch_compute(GLuint program, GLuint x, GLuint y, GLuint z) noexcept;
void dispatch_compute(GLuint program, GLuint x, GLuint y, GLuint z, GLbitfield barriers) noexcept;

} // namespace surge::gl_atom::shader

#endif // SURGE_CORE_GL_ATOM_SHADERS_HPP