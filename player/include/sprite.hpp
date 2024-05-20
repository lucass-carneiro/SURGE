#ifndef SURGE_ATOM_SPRITE_HPP
#define SURGE_ATOM_SPRITE_HPP

#include "error_types.hpp"
#include "gl_includes.hpp"
#include "gpu_bump_array.hpp"

#include <glm/glm.hpp>
#include <tl/expected.hpp>

namespace surge::atom::sprite {

struct database {
#ifdef SURGE_BUILD_TYPE_Debug
public:
#else
private:
#endif
  GLuint sprite_shader{0};
  GLuint deep_sprite_shader{0};

  GLuint VBO{0};
  GLuint EBO{0};
  GLuint VAO{0};

  // Depth sprite
  GLuint64 depth_texture_handle{0};
  GLuint64 depth_map_handle{0};
  glm::mat4 depth_model{1.0f};

  // Regular sprites
  gba<GLuint64> texture_handles{};
  gba<glm::mat4> models{};
  gba<float> alphas{};

public:
  static auto create(usize max_sprites) noexcept -> tl::expected<database, error>;
  void destroy() noexcept;

  void add(GLuint64 handle, glm::mat4 model, float alpha) noexcept;
  void add_depth(GLuint64 texture, GLuint64 depth_map, glm::mat4 model) noexcept;

  void reset() noexcept;
  void reinit() noexcept;

  void translate(usize idx, const glm::vec3 &dir) noexcept;
  auto get_pos(usize idx) noexcept -> glm::vec3;

  void draw() noexcept;
};

auto place(glm::vec2 &&pos, glm::vec2 &&scale, float z = 0.0f) noexcept -> glm::mat4;
auto place(const glm::vec2 &pos, const glm::vec2 &scale, float z = 0.0f) noexcept -> glm::mat4;

} // namespace surge::atom::sprite

#endif // SURGE_ATOM_SPRITE_HPP