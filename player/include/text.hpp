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

struct buffer_data {
  GLuint VBO;
  GLuint EBO;
  GLuint VAO;

  // SSBOs
  GLuint MMB; // model matrices buffer
  GLuint THB; // texture handles buffer
};

struct glyph_data {
  vector<GLuint> texture_id;
  vector<GLuint64> texture_handle;
  vector<u32> bitmap_width;
  vector<u32> bitmap_height;
  vector<i32> bearing_x;
  vector<i32> bearing_y;
  vector<i64> advance;
  i64 whitespace_advance;
  i64 line_spacing;
};

struct text_draw_data {
  vector<GLuint64> texture_handles;
  vector<glm::mat4> glyph_models;
  glm::vec4 bounding_box;
  glm::vec4 color;
};

// The original Twitter limit
auto create_buffers(usize max_chars = 140) noexcept -> buffer_data;
void destroy_buffers(const buffer_data &) noexcept;

auto init_freetype() noexcept -> tl::expected<FT_Library, error>;
auto destroy_freetype(FT_Library lib) noexcept -> std::optional<error>;

auto load_face(FT_Library lib, const char *name) noexcept -> tl::expected<FT_Face, error>;
auto unload_face(FT_Face face) noexcept -> std::optional<error>;

auto load_glyphs(FT_Library lib, FT_Face face, FT_UInt pixel_size = 16) noexcept
    -> tl::expected<glyph_data, error>;
void unload_glyphs(glyph_data &gd) noexcept;

void make_glyphs_resident(glyph_data &gd);
void make_glyphs_non_resident(glyph_data &gd);

void send_buffers(const buffer_data &bd, const text_draw_data &tdd) noexcept;

void append_text_draw_data(text_draw_data &tdd, const glyph_data &gd, std::string_view text,
                           const glm::vec3 &baseline_origin, const glm::vec4 &color,
                           const glm::vec2 &scale = glm::vec2{1.0f}) noexcept;

void draw(const GLuint &sp, const buffer_data &bd, const GLuint &MPSB,
          const text_draw_data &tdd) noexcept;

} // namespace surge::atom::text

#endif // SURGE_ATOM_TEXT