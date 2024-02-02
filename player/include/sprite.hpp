#ifndef SURGE_ATOM_SPRITE_HPP
#define SURGE_ATOM_SPRITE_HPP

#include "container_types.hpp"
#include "files.hpp"
#include "renderer.hpp"

#include <glm/glm.hpp>
#include <tl/expected.hpp>

namespace surge::atom::sprite {

struct buffer_data {
  GLuint VBO;
  GLuint EBO;
  GLuint VAO;
};

auto create_buffers() noexcept -> buffer_data;
void destroy_buffers(const buffer_data &) noexcept;

auto create_texture(const files::image &image,
                    renderer::texture_filtering filtering = renderer::texture_filtering::linear,
                    renderer::texture_wrap wrap = renderer::texture_wrap::clamp_to_edge) noexcept
    -> tl::expected<GLuint64, error>;

void make_resident(GLuint64 handle) noexcept;
void make_non_resident(GLuint64 handle) noexcept;

void make_resident(const vector<GLuint64> &texture_handles) noexcept;
void make_non_resident(const vector<GLuint64> &texture_handles) noexcept;

void draw(const GLuint &sp, const buffer_data &bd, const glm::mat4 &proj, const glm::mat4 &view,
          const vector<glm::mat4> &models, const vector<GLuint64> &texture_handles,
          const vector<float> &alphas) noexcept;

void draw_hv_flip(const GLuint &sp, const buffer_data &bd, const glm::mat4 &proj,
                  const glm::mat4 &view, const vector<glm::mat4> &models,
                  const vector<GLuint64> &texture_handles, const vector<float> &alphas) noexcept;

} // namespace surge::atom::sprite

#endif // SURGE_ATOM_SPRITE_HPP