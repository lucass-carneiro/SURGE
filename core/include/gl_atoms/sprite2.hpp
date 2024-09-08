#ifndef SURGE_CORE_GL_ATOM_PMB_HPP
#define SURGE_CORE_GL_ATOM_PMB_HPP

#include "error_types.hpp"
#include "integer_types.hpp"
#include "renderer_gl.hpp"

#include <glm/glm.hpp>
#include <tl/expected.hpp>

namespace surge::gl_atom::sprite2 {

struct database_create_info {
  usize max_sprites{0};       // The size of a single element in the buffer
  usize buffer_redundancy{3}; // The number of buffers to cycle trough
};

struct database_t;
using database = database_t *;

auto database_create(database_create_info ci) noexcept -> tl::expected<database, error>;
void database_destroy(database sdb) noexcept;

void database_add(database sdb, GLuint64 texture_handle, const glm::mat4 &model_matrix,
                  float alpha) noexcept;

void draw(database sdb) noexcept;

} // namespace surge::gl_atom::sprite2

#endif // SURGE_CORE_GL_ATOM_PMB_HPP