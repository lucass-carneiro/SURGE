#ifndef SURGE_ATOM_TEXT
#define SURGE_ATOM_TEXT

#include "allocators.hpp"
#include "renderer.hpp"

// clang-format off
#include <ft2build.h>
#include <glm/fwd.hpp>
#include <tl/expected.hpp>
#include FT_FREETYPE_H

#include <freetype/ftimage.h>
#include <freetype/fttypes.h>
// clang-format on

#include <EASTL/vector.h>
#include <glm/glm.hpp>
#include <string>
#include <string_view>

/**
 * Drawable text
 */
namespace surge::atom::text {

using font_name_vec_t = eastl::vector<std::string, allocators::eastl::gp_allocator>;
using face_vec_t = eastl::vector<FT_Face, allocators::eastl::gp_allocator>;
using tid_vec_t = eastl::vector<GLuint, allocators::eastl::gp_allocator>;
using size_vec_t = eastl::vector<FT_UInt, allocators::eastl::gp_allocator>;
using bea_vec_t = eastl::vector<FT_Int, allocators::eastl::gp_allocator>;
using adv_vec_t = eastl::vector<FT_Pos, allocators::eastl::gp_allocator>;

enum class error : std::uint32_t {
  freetype_init = 1,
  freetype_face_not_found = 2,
  freetype_set_face_size = 3,
  freetype_character_load = 4
};

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

void draw(GLuint shader_program, const buffer_data &bd, const charmap_data &cd, const draw_data &dd,
          std::string_view text, float extra_vskip = 5.0f) noexcept;

void draw(GLuint shader_program, const buffer_data &bd, const charmap_data &cd, const draw_data &dd,
          unsigned long long number) noexcept;

} // namespace surge::atom::text

#endif // SURGE_ATOM_TEXT