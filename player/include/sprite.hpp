#ifndef SURGE_ATOM_SPRITE_HPP
#define SURGE_ATOM_SPRITE_HPP

#include "container_types.hpp"
#include "gl_includes.hpp"

#include <glm/glm.hpp>
#include <tl/expected.hpp>

namespace surge::atom::sprite {

struct database {
private:
  GLuint VBO{0};
  GLuint EBO{0};
  GLuint VAO{0};

  GLuint MMB{0}; // Model matrices buffer
  GLuint AVB{0}; // Alpha value buffer
  GLuint THB{0}; // Texture handles buffer

  vector<GLuint64> texture_handles;
  vector<glm::mat4> models;
  vector<float> alphas;

public:
  static auto create(usize max_sprites) noexcept -> database;
  void destroy() noexcept;

  void add(GLuint64 handle, glm::mat4 model, float alpha) noexcept;
  void reset() noexcept;
  void update() noexcept;

  void draw(const GLuint &sp) noexcept;
};

auto place(glm::vec2 &&pos, glm::vec2 &&scale, float z = 0.0f) noexcept -> glm::mat4;
auto place(const glm::vec2 &pos, const glm::vec2 &scale, float z = 0.0f) noexcept -> glm::mat4;

} // namespace surge::atom::sprite

#endif // SURGE_ATOM_SPRITE_HPP