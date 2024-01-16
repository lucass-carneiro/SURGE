#ifndef SURGE_ATOM_STATIC_IMAGE
#define SURGE_ATOM_STATIC_IMAGE

#include "error_types.hpp"
#include "renderer.hpp"

#include <glm/fwd.hpp>
#include <tl/expected.hpp>

/**
 * Drawable static image quad
 */
namespace surge::atom::static_image {

struct one_buffer_data {
  glm::vec2 dimentions{0.0f};
  glm::vec2 ds{0.0f};

  GLuint texture_id;
  GLuint VBO;
  GLuint EBO;
  GLuint VAO;
};

struct one_draw_data {
  glm::mat4 projection;
  glm::mat4 view;
  glm::vec3 pos;
  glm::vec3 scale;
  glm::vec2 region_origin;
  glm::vec2 region_dims;
  bool h_flip{false};
  bool v_flip{false};
};

struct image_data {
  unsigned char *data{nullptr};
  int iw{0};
  int ih{0};
  int channels_in_file{0};
};

using load_image_t = tl::expected<image_data, error>;
using create_t = tl::expected<one_buffer_data, error>;

auto load_image(const char *p) noexcept -> load_image_t;

auto make_texture(image_data &img_data, renderer::texture_filtering filtering
                                        = renderer::texture_filtering::linear) noexcept
    -> one_buffer_data;

auto create(const char *p, renderer::texture_filtering filtering
                           = renderer::texture_filtering::linear) noexcept -> create_t;

void draw(GLuint shader_program, const one_buffer_data &ctx, const one_draw_data &dctx) noexcept;
void draw(GLuint shader_program, const one_buffer_data &&ctx, one_draw_data &&dctx) noexcept;

void cleanup(one_buffer_data &ctx) noexcept;

} // namespace surge::atom::static_image

#endif // SURGE_ATOM_STATIC_IMAGE