#ifndef SURGE_CORE_GL_ATOM_PMB_HPP
#define SURGE_CORE_GL_ATOM_PMB_HPP

#include "error_types.hpp"
#include "integer_types.hpp"
#include "renderer_gl.hpp"

#include <glm/glm.hpp>
#include <tl/expected.hpp>

namespace surge::gl_atom::sprite_database {

struct database_create_info {
  usize max_sprites{0};       // The size of a single element in the buffer
  usize buffer_redundancy{3}; // The number of buffers to cycle trough
};

struct database_t;
using database = database_t *;

auto create(database_create_info ci) noexcept -> tl::expected<database, error>;
void destroy(database sdb) noexcept;

void wait_idle(database sdb) noexcept;

void begin_add(database sdb) noexcept;

void add(database sdb, GLuint64 texture_handle, const glm::mat4 &model_matrix,
         float alpha) noexcept;
void add(database sdb, GLuint64 texture_handle, glm::vec2 &&pos, glm::vec2 &&scale, float z,
         float alpha) noexcept;
void add(database sdb, GLuint64 texture_handle, const glm::vec2 &pos, const glm::vec2 &scale,
         float z, float alpha) noexcept;

void add_view(database sdb, GLuint64 handle, glm::mat4 model, glm::vec4 image_view,
              glm::vec2 img_dims, float alpha) noexcept;
void add_view(database sdb, GLuint64 handle, glm::vec2 &&pos, glm::vec2 &&scale, float z,
              glm::vec4 image_view, glm::vec2 img_dims, float alpha) noexcept;
void add_view(database sdb, GLuint64 handle, const glm::vec2 &pos, const glm::vec2 &scale, float z,
              glm::vec4 image_view, glm::vec2 img_dims, float alpha) noexcept;

void add_depth(database sdb, GLuint64 texture, GLuint64 depth_map, glm::mat4 model) noexcept;

void draw(database sdb) noexcept;

} // namespace surge::gl_atom::sprite_database

#endif // SURGE_CORE_GL_ATOM_PMB_HPP