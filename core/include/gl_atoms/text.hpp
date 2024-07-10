#ifndef SURGE_CORE_GL_ATOM_TEXT_HPP
#define SURGE_CORE_GL_ATOM_TEXT_HPP

#include "container_types.hpp"
#include "error_types.hpp"
#include "gba.hpp"

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
 * @brief Draw Text on the screen. For now, we handle only the printable ASCII set, but the
 * infrastructure would allow for more characters to be handled. Maybe.
 *
 */
namespace surge::gl_atom {

struct text_engine {
private:
  FT_Library ft_library{nullptr};
  hash_map<std::string_view, FT_Face> faces{};

public:
  static auto create() noexcept -> tl::expected<text_engine, error>;
  void destroy() noexcept;

  auto load_face(const char *path, std::string_view, FT_F26Dot6 size_in_pts = 40,
                 FT_UInt resolution_dpi = 300) noexcept -> std::optional<error>;

  [[nodiscard]] auto get_face(std::string_view name) noexcept -> std::optional<FT_Face>;
  [[nodiscard]] auto get_faces() noexcept -> hash_map<std::string_view, FT_Face> &;
};

enum language { english };

struct glyph_cache {
private:
  hash_map<FT_ULong, GLuint> texture_ids{};
  hash_map<FT_ULong, GLuint64> texture_handles{};

  hash_map<FT_ULong, glm::vec<2, unsigned int>> bitmap_dims{};

  hash_map<FT_ULong, glm::vec<2, FT_Int>> bearings{};
  hash_map<FT_ULong, glm::vec<2, FT_Pos>> advances{};

public:
  static auto create(FT_Face face, language = english) noexcept -> tl::expected<glyph_cache, error>;
  void destroy() noexcept;

  auto load_character(FT_Face face, FT_ULong c) noexcept -> std::optional<error>;
  auto load_nonprintable_character(FT_Face face, FT_ULong c) noexcept -> std::optional<error>;

  void make_resident() noexcept;
  void make_non_resident() noexcept;

  [[nodiscard]] auto get_texture_handles() const noexcept -> const hash_map<FT_ULong, GLuint64> &;

  [[nodiscard]] auto
  get_bitmap_dims() const noexcept -> const hash_map<FT_ULong, glm::vec<2, unsigned int>> &;

  [[nodiscard]] auto
  get_bearings() const noexcept -> const hash_map<FT_ULong, glm::vec<2, FT_Int>> &;

  [[nodiscard]] auto
  get_advances() const noexcept -> const hash_map<FT_ULong, glm::vec<2, FT_Pos>> &;
};

struct text_buffer {
private:
  GLuint text_shader{0};

  GLuint VBO{0};
  GLuint EBO{0};
  GLuint VAO{0};

  gba<glm::mat4> models{};
  gba<GLuint64> texture_handles{};

public:
  static auto create(usize max_chars) noexcept -> tl::expected<text_buffer, error>;
  void destroy() noexcept;

  auto get_bbox_size(glyph_cache &cache, std::string_view text) noexcept -> glm::vec2;

  void push(const glm::vec3 &baseline_origin, const glm::vec2 &scale, glyph_cache &cache,
            std::string_view text) noexcept;

  void push_centered(const glm::vec3 &baseline_origin, float intended_scale,
                     const glm::vec2 &region_dims, glyph_cache &cache, std::string_view text,
                     float decrement_step = 1.0e-2f) noexcept;

  void reset() noexcept;

  void draw(const glm::vec4 &color) noexcept;
};

} // namespace surge::gl_atom

#endif // SURGE_CORE_GL_ATOM_TEXT_HPP