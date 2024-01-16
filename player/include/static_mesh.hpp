#ifndef SURGE_ATOM_STATIC_MESH_HPP
#define SURGE_ATOM_STATIC_MESH_HPP

#include "error_types.hpp"
#include "integer_types.hpp"
#include "renderer.hpp"

/**
 * Drawable mesh with the following properties:
 *
 * Dynamic geometry: No
 * Textured: No
 * RGB colored: Yes
 */
namespace surge::atom::static_mesh {

struct one_buffer_data {
  GLuint VAO;
  GLsizei elements;
};

struct one_draw_data {
  glm::mat4 projection;
  glm::mat4 view;
  glm::mat4 model;
  glm::vec4 color;
};

auto gen_triangle() noexcept -> std::tuple<GLuint, usize>;
auto gen_square() noexcept -> std::tuple<GLuint, usize>;

auto load(const char *path) noexcept -> tl::expected<one_buffer_data, error>;

void draw(GLuint shader_program, const one_buffer_data &bd, const one_draw_data &dd) noexcept;

} // namespace surge::atom::static_mesh

#endif // SURGE_ATOM_STATIC_MESH_HPP