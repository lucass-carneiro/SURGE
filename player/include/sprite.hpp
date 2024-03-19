#ifndef SURGE_ATOM_SPRITE_HPP
#define SURGE_ATOM_SPRITE_HPP

#include "container_types.hpp"
#include "files.hpp"
#include "renderer.hpp"
#include "tasks.hpp"

#include <glm/glm.hpp>
#include <tl/expected.hpp>
#include <tuple>

namespace surge::atom::sprite {

class record {
public:
  record(usize max_sprts = 32);

  void sync_buffers() noexcept;

  void add(glm::mat4 &&model_matrix, GLuint64 &&texture_handle, float &&alpha) noexcept;

  void draw(const GLuint &sp, const GLuint &MPSB) noexcept;

  void reset() noexcept;
  void create_buffers() noexcept;
  void destroy_buffers() noexcept;

private:
  const usize max_sprites{0};
  GLuint VBO{0};
  GLuint EBO{0};
  GLuint VAO{0};

  GLuint MMB{0}; // Model matrices buffer
  GLuint AVB{0}; // Alpha value buffer
  GLuint THB{0}; // Texture handles buffer

  vector<GLuint64> texture_handles;
  vector<glm::mat4> models;
  vector<float> alphas;
};

struct buffer_data {
  GLuint VBO;
  GLuint EBO;
  GLuint VAO;

  // SSBOs
  GLuint MMB; // model matrices buffer
  GLuint AVB; // alpha value buffer
  GLuint THB; // texture handles buffer
};

struct data_list {
  surge::vector<GLuint64> texture_handles;
  surge::vector<glm::mat4> models;
  surge::vector<float> alphas;
};

} // namespace surge::atom::sprite

#endif // SURGE_ATOM_SPRITE_HPP