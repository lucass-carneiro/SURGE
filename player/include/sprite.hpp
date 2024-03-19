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
  record(usize max_sprites);

  void sync_buffers() noexcept;

  void draw(const GLuint &sp, const GLuint &MPSB) noexcept;

  void reset() noexcept;
  void destroy_buffers() noexcept;

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

auto create_buffers(usize max_sprites = 32) noexcept -> buffer_data;
void destroy_buffers(const buffer_data &) noexcept;

auto create_texture(const files::image &image,
                    renderer::texture_filtering filtering = renderer::texture_filtering::linear,
                    renderer::texture_wrap wrap = renderer::texture_wrap::clamp_to_edge) noexcept
    -> tl::expected<std::tuple<GLuint, GLuint64>, error>;

void destroy_texture(GLuint texture) noexcept;
void destroy_texture(const vector<GLuint> &texture) noexcept;

void make_resident(GLuint64 handle) noexcept;
void make_non_resident(GLuint64 handle) noexcept;

void make_resident(const vector<GLuint64> &texture_handles) noexcept;
void make_non_resident(const vector<GLuint64> &texture_handles) noexcept;
auto is_resident(GLuint64 handle) noexcept -> bool;

void send_buffers(const buffer_data &bd, const data_list &dl) noexcept;

void draw(const GLuint &sp, const buffer_data &bd, const GLuint &MPSB,
          const data_list &dl) noexcept;

} // namespace surge::atom::sprite

#endif // SURGE_ATOM_SPRITE_HPP