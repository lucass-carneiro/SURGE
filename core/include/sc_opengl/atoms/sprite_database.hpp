#ifndef SURGE_CORE_GL_ATOM_PMB_HPP
#define SURGE_CORE_GL_ATOM_PMB_HPP

#include "sc_error_types.hpp"
#include "sc_integer_types.hpp"
#include "sc_opengl/sc_opengl.hpp"
#include "sc_options.hpp"

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
         const glm::vec4 &color_mod = glm::vec4{1.0f}) noexcept;
void add(database sdb, GLuint64 texture_handle, glm::vec2 &&pos, glm::vec2 &&scale, float z,
         glm::vec4 &&color_mod = glm::vec4{1.0f}) noexcept;
void add(database sdb, GLuint64 texture_handle, const glm::vec2 &pos, const glm::vec2 &scale,
         float z, const glm::vec4 &color_mod = glm::vec4{1.0f}) noexcept;

void add_view(database sdb, GLuint64 handle, glm::mat4 model, glm::vec4 image_view,
              glm::vec2 img_dims, const glm::vec4 &color_mod = glm::vec4{1.0f}) noexcept;
void add_view(database sdb, GLuint64 handle, glm::vec2 &&pos, glm::vec2 &&scale, float z,
              glm::vec4 image_view, glm::vec2 img_dims,
              glm::vec4 &&color_mod = glm::vec4{1.0f}) noexcept;
void add_view(database sdb, GLuint64 handle, const glm::vec2 &pos, const glm::vec2 &scale, float z,
              glm::vec4 image_view, glm::vec2 img_dims,
              const glm::vec4 &color_mod = glm::vec4{1.0f}) noexcept;

void add_depth(database sdb, GLuint64 texture, GLuint64 depth_map, glm::mat4 model) noexcept;

void draw(database sdb) noexcept;

auto place_sprite(glm::vec2 &&pos, glm::vec2 &&scale, float z) noexcept -> glm::mat4;
auto place_sprite(const glm::vec2 &pos, const glm::vec2 &scale, float z) noexcept -> glm::mat4;

#ifdef SURGE_BUILD_TYPE_Debug
auto get_sprites_in_buffer(database sdb) noexcept -> usize;
auto get_current_buffer_idx(database sdb) noexcept -> usize;
#endif

} // namespace surge::gl_atom::sprite_database

#endif // SURGE_CORE_GL_ATOM_PMB_HPP