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

void database_wait_idle(database sdb) noexcept;

void database_begin_add(database sdb) noexcept;

void database_add(database sdb, GLuint64 texture_handle, const glm::mat4 &model_matrix,
                  float alpha) noexcept;

void database_add_view(database sdb, GLuint64 handle, glm::mat4 model, glm::vec4 image_view,
                       glm::vec2 img_dims, float alpha) noexcept;

void database_draw(database sdb) noexcept;

} // namespace surge::gl_atom::sprite2

#endif // SURGE_CORE_GL_ATOM_PMB_HPP