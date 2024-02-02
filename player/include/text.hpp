#ifndef SURGE_ATOM_TEXT
#define SURGE_ATOM_TEXT

#include "container_types.hpp"
#include "renderer.hpp"
#include "sprite.hpp"

// clang-format off
#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftimage.h>
#include <freetype/fttypes.h>
// clang-format on

#include <glm/glm.hpp>
#include <optional>
#include <string_view>
#include <tl/expected.hpp>

/**
 * Drawable text
 */
namespace surge::atom::text {

struct glyph_data {
  vector<GLuint> texture_id;
  vector<GLuint64> texture_handle;
  vector<u32> bitmap_width;
  vector<u32> bitmap_height;
  vector<i32> bearing_x;
  vector<i32> bearing_y;
  vector<i64> advance;
  i64 whitespace_advance;
};

struct text_draw_data {
  vector<GLuint64> texture_handles;
  vector<glm::mat4> glyph_models;
};

auto init_freetype() noexcept -> tl::expected<FT_Library, error>;
auto destroy_freetype(FT_Library lib) noexcept -> std::optional<error>;

auto load_face(FT_Library lib, const char *name) noexcept -> tl::expected<FT_Face, error>;
auto unload_face(FT_Face face) noexcept -> std::optional<error>;

auto load_glyphs(FT_Library lib, FT_Face face, FT_UInt pixel_size = 16) noexcept
    -> tl::expected<glyph_data, error>;
void unload_glyphs(glyph_data &gd) noexcept;

void make_glyphs_resident(glyph_data &gd);
void make_glyphs_non_resident(glyph_data &gd);

auto create_text_draw_data(const glyph_data &gd, std::string_view text,
                           glm::vec3 &&baseline_origin) noexcept -> text_draw_data;

void draw(const GLuint &sp, const sprite::buffer_data &bd, const glm::mat4 &proj,
          const glm::mat4 &view, const text_draw_data &tdd, glm::vec4 &&color) noexcept;

} // namespace surge::atom::text

#endif // SURGE_ATOM_TEXT