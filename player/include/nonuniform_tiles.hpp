#ifndef SURGE_ATOM_NONUNIFORM_TILES_HPP
#define SURGE_ATOM_NONUNIFORM_TILES_HPP

#include "container_types.hpp"
#include "error_types.hpp"
#include "integer_types.hpp"
#include "renderer.hpp"

#include <glm/glm.hpp>
#include <tl/expected.hpp>

/**
 * Multiple textured quads.
 * Each quad can have their own shape.
 * Each quad can have their own texture, loaded from an atlas.
 * Each texture in the atlas must be OF THE SAME SIZE
 */
namespace surge::atom::nonuniform_tiles {

// Keep it in sync with the shader
constexpr const usize max_tiles{8};

struct buffer_data {
  GLuint texture_id;
  GLuint VBO;
  GLuint EBO;
  GLuint VAO;
};

struct draw_data {
  glm::mat4 projection;
  glm::mat4 view;
  surge::vector<glm::mat4> models;
};

// Tiles need to be in a vertical strip
struct tile_structure {
  const char *file{nullptr};
  GLsizei num_tiles{0};
  renderer::texture_filtering filtering{renderer::texture_filtering::nearest};
};

auto create(const tile_structure &structure) noexcept -> tl::expected<buffer_data, error>;

void draw(GLuint shader_program, const buffer_data &ctx, const draw_data &dctx) noexcept;

void cleanup(buffer_data &ctx) noexcept;

} // namespace surge::atom::nonuniform_tiles

#endif // SURGE_ATOM_NONUNIFORM_TILES_HPP