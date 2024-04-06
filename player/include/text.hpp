#ifndef SURGE_ATOM_TEXT
#define SURGE_ATOM_TEXT

#include "container_types.hpp"
#include "error_types.hpp"
#include "gpu_bump_array.hpp"

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

struct text_engine {
private:
  FT_Library ft_library{nullptr};
  hash_map<const char *, FT_Face> faces{};

public:
  static auto create() noexcept -> tl::expected<text_engine, error>;
  void destroy() noexcept;

  auto load_face(const char *path, const char *name, FT_F26Dot6 size_in_pts = 40,
                 FT_UInt resolution_dpi = 300) noexcept -> std::optional<error>;

  [[nodiscard]] auto get_faces() noexcept -> hash_map<const char *, FT_Face> &;
};

enum language { english, portuguese };

struct glyph_cache {
private:
  hash_map<FT_ULong, GLuint> texture_ids{};
  hash_map<FT_ULong, GLuint64> texture_handles{};

  hash_map<FT_ULong, glm::vec<2, unsigned int>> bitmap_dims{};

  hash_map<FT_ULong, glm::vec<2, FT_Int>> bearings{};
  hash_map<FT_ULong, glm::vec<2, FT_Pos>> advances{};

public:
  static auto create(FT_Face face, language lang = english) noexcept
      -> tl::expected<glyph_cache, error>;
  void destroy() noexcept;

  auto load_character(FT_Face face, FT_ULong c) noexcept -> std::optional<error>;

  void make_resident() noexcept;
  void make_non_resident() noexcept;
};

struct text_buffer {
private:
  GLuint VBO{0};
  GLuint EBO{0};
  GLuint VAO{0};

  gba<GLuint64> texture_handles{};
  gba<glm::mat4> models{};

public:
  static auto create(usize max_chars = 540) noexcept -> text_buffer;
  void destroy() noexcept;

  void add(glyph_cache &cache, std::string_view text) noexcept;
  void reset() noexcept;

  void draw() noexcept;
};

class record {
public:
  record();

private:
  GLuint VBO;
  GLuint EBO;
  GLuint VAO;

  // SSBOs
  GLuint MMB; // model matrices buffer
  GLuint THB; // texture handles buffer
};

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

auto load_glyphs(FT_Library, FT_Face face, FT_F26Dot6 size_in_pts = 40,
                 FT_UInt resolution_dpi = 300) noexcept -> tl::expected<glyph_data, error>;
void unload_glyphs(glyph_data &gd) noexcept;

void make_glyphs_resident(glyph_data &gd);
void make_glyphs_non_resident(glyph_data &gd);

void send_buffers(const buffer_data &bd, const text_draw_data &tdd) noexcept;

void append_text_draw_data(text_draw_data &tdd, const glyph_data &gd, std::string_view text,
                           const glm::vec3 &baseline_origin, const glm::vec4 &color,
                           const glm::vec2 &scale = glm::vec2{1.0f}) noexcept;

void append_text_draw_data(text_draw_data &tdd, const glyph_data &gd, u8 value,
                           const glm::vec3 &baseline_origin, const glm::vec4 &color,
                           const glm::vec2 &scale = glm::vec2{1.0f}) noexcept;

void draw(const GLuint &sp, const buffer_data &bd, const GLuint &MPSB,
          const text_draw_data &tdd) noexcept;

} // namespace surge::atom::text

#endif // SURGE_ATOM_TEXT