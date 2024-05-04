#ifndef SURGE_ATOM_SPRITE_HPP
#define SURGE_ATOM_SPRITE_HPP

#include "container_types.hpp"
#include "gl_includes.hpp"
// #include "gl_ring_buffer.hpp"
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
  GLuint VBO{0};
  GLuint EBO{0};
  GLuint VAO{0};

  gba<GLuint64> texture_handles{};
  gba<glm::mat4> models{};
  gba<float> alphas{};

public:
  static auto create(usize max_sprites) noexcept -> database;
  void destroy() noexcept;

  void add(GLuint64 handle, glm::mat4 model, float alpha) noexcept;

  void reset() noexcept;
  void reinit() noexcept;

  void draw(const GLuint &sp) noexcept;
};

auto place(glm::vec2 &&pos, glm::vec2 &&scale, float z = 0.0f) noexcept -> glm::mat4;
auto place(const glm::vec2 &pos, const glm::vec2 &scale, float z = 0.0f) noexcept -> glm::mat4;

} // namespace surge::atom::sprite

#endif // SURGE_ATOM_SPRITE_HPP