#ifndef SURGE_ATOM_TEXT
#define SURGE_ATOM_TEXT

#include "container_types.hpp"
#include "renderer.hpp"

// clang-format off
#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftimage.h>
#include <freetype/fttypes.h>
// clang-format on

#include <tl/expected.hpp>

/**
 * Drawable text
 */
namespace surge::atom::text {

using font_name_vec_t = vector<string>;
using face_vec_t = vector<FT_Face>;
using tid_vec_t = vector<GLuint>;
using size_vec_t = vector<FT_UInt>;
using bea_vec_t = vector<FT_Int>;
using adv_vec_t = vector<FT_Pos>;

struct buffer_data {
  FT_Library library;
  face_vec_t faces;
  GLuint VAO;
  GLuint VBO;
};

struct charmap_data {
  std::size_t chars_per_face;

  tid_vec_t texture_ids;
  size_vec_t sizes_x;
  size_vec_t sizes_y;
  bea_vec_t bearings_x;
  bea_vec_t bearings_y;
  adv_vec_t advances;
};

struct draw_data {
  glm::mat4 projection;
  std::uint64_t face_idx;
  glm::vec2 position;
  float scale;
  glm::vec3 color;
};

auto create(const font_name_vec_t &fonts) noexcept -> tl::expected<buffer_data, error>;
void terminate(buffer_data &data) noexcept;

auto create_charmap(buffer_data &data, FT_UInt pixel_height,
                    renderer::texture_filtering filtering
                    = renderer::texture_filtering::linear) noexcept
    -> tl::expected<charmap_data, error>;

void destroy_charmap(const charmap_data &charmap) noexcept;

void draw(GLuint shader_program, const buffer_data &bd, const charmap_data &cd, const draw_data &dd,
          std::string_view text, float extra_vskip = 5.0f) noexcept;

void draw(GLuint shader_program, const buffer_data &bd, const charmap_data &cd, const draw_data &dd,
          unsigned long long number) noexcept;

} // namespace surge::atom::text

#endif // SURGE_ATOM_TEXT