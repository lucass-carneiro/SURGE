#include "text.hpp"

#include "allocators.hpp"
#include "logging.hpp"

// clang-format off
#include <freetype/freetype.h>
#include <freetype/ftsystem.h>
#include <freetype/ftmodapi.h>
// clang-format on

#include <glm/gtc/matrix_transform.hpp>
#include <gsl/gsl-lite.hpp>
#include <tl/expected.hpp>

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#  include <tracy/TracyOpenGL.hpp>
#endif

static auto FT_malloc(FT_Memory, long size) noexcept -> void * {
  return surge::allocators::mimalloc::malloc(static_cast<surge::usize>(size));
}

static void FT_free(FT_Memory, void *block) noexcept { surge::allocators::mimalloc::free(block); }

static auto FT_realloc(FT_Memory, long, long new_size, void *block) noexcept -> void * {
  return surge::allocators::mimalloc::realloc(block, static_cast<surge::usize>(new_size));
}

// NOLINTNEXTLINE
static FT_MemoryRec_ ft_mimalloc{nullptr, FT_malloc, FT_free, FT_realloc};

auto surge::atom::text::init_freetype() noexcept -> tl::expected<FT_Library, error> {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::text::init_freetype()");
#endif
  log_info("Creating FreeType library");

  FT_Library lib{};
  auto status{FT_New_Library(&ft_mimalloc, &lib)};

  if (status != 0) {
    log_error("Error creating FreeType library: %s", FT_Error_String(status));
    return tl::unexpected{error::freetype_init};
  }

  log_info("Adding FreeType modules");
  FT_Add_Default_Modules(lib);

  return lib;
}

auto surge::atom::text::destroy_freetype(FT_Library lib) noexcept -> std::optional<error> {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::text::destroy_freetype()");
#endif

  log_info("Destroying FreeType library");

  const auto status{FT_Done_Library(lib)};

  if (status != 0) {
    log_error("Error destroying FreeType library: %s", FT_Error_String(status));
    return error::freetype_deinit;
  } else {
    return {};
  }
}

auto surge::atom::text::load_face(FT_Library lib, const char *name) noexcept
    -> tl::expected<FT_Face, error> {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::text::append_face()");
#endif

  log_info("Loading face %s", name);

  FT_Face face{};
  const auto status{FT_New_Face(lib, name, 0, &face)};

  if (status != 0) {
    log_error("Error loading face %s: %s", name, FT_Error_String(status));
    return tl::unexpected{error::freetype_face_load};
  } else {
    return face;
  }
}

auto surge::atom::text::unload_face(FT_Face face) noexcept -> std::optional<error> {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::text::unload_face()");
#endif

  log_info("Unloading face %s", face->family_name);

  const auto status = FT_Done_Face(face);
  if (status != 0) {
    log_error("Error unloading face %s: %s", face->family_name, FT_Error_String(status));
    return error::freetype_face_unload;
  } else {
    return {};
  }
}

auto surge::atom::text::load_glyphs(FT_Library, FT_Face face, FT_UInt pixel_size) noexcept
    -> tl::expected<glyph_data, error> {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::text::load_glyphs()");
  TracyGpuZone("GPU surge::atom::text::load_glyphs()");
#endif

  log_info("Loading glyphs metrics in %s", face->family_name);

  auto status{FT_Set_Pixel_Sizes(face, 0, pixel_size)};
  if (status != 0) {
    log_error("Unable to set face %s glyphs to size: %s", face->family_name,
              FT_Error_String(status));
    return tl::unexpected{error::freetype_set_face_size};
  }

  glyph_data gd;
  gd.texture_id.reserve(94);
  gd.texture_handle.reserve(94);
  gd.bitmap_width.reserve(94);
  gd.bitmap_height.reserve(94);
  gd.bearing_x.reserve(94);
  gd.bearing_y.reserve(94);
  gd.advance.reserve(94);

  // White space
  status = FT_Load_Char(face, ' ', FT_LOAD_RENDER);
  if (status != 0) {
    log_error("Unable to load white space ASCII character for face %s: %s", face->family_name,
              FT_Error_String(status));
    return tl::unexpected{error::freetype_character_load};
  } else {
    gd.whitespace_advance = face->glyph->advance.x;
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  // Printable ASCII characters
  for (FT_ULong c = 33; c <= 126; c++) {
    status = FT_Load_Char(face, c, FT_LOAD_RENDER);
    if (status != 0) {
      log_error("Unable to load ASCII character %c for face %s: %s", static_cast<char>(c),
                face->family_name, FT_Error_String(status));
      return tl::unexpected{error::freetype_character_load};
    } else {

      const auto bw{face->glyph->bitmap.width};
      const auto bh{face->glyph->bitmap.rows};

      gd.bitmap_width.push_back(bw);
      gd.bitmap_height.push_back(bh);
      gd.bearing_x.push_back(face->glyph->bitmap_left);
      gd.bearing_y.push_back(face->glyph->bitmap_top);
      gd.advance.push_back(face->glyph->advance.x);

      GLuint texture{0};
      glCreateTextures(GL_TEXTURE_2D, 1, &texture);

      // Wrapping
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

      // Filtering
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      constexpr const auto internal_format{GL_R8};
      constexpr const auto format{GL_RED};

      glTextureStorage2D(texture, 1, internal_format, static_cast<GLsizei>(bw),
                         static_cast<GLsizei>(bh));
      glTextureSubImage2D(texture, 0, 0, 0, static_cast<GLsizei>(bw), static_cast<GLsizei>(bh),
                          format, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);

      const auto handle{glGetTextureHandleARB(texture)};
      if (handle == 0) {
        log_error("Unable to create texture handle for character %c", static_cast<char>(c));
        return tl::unexpected{error::texture_handle_creation};
      } else {
        gd.texture_id.push_back(texture);
        gd.texture_handle.push_back(handle);
      }
    }
  }

  return gd;
}

void surge::atom::text::unload_glyphs(glyph_data &gd) noexcept {
  glDeleteTextures(static_cast<GLsizei>(gd.texture_id.size()), gd.texture_id.data());

  gd.texture_id.clear();
  gd.texture_handle.clear();
  gd.bitmap_width.clear();
  gd.bitmap_height.clear();
  gd.bearing_x.clear();
  gd.bearing_y.clear();
  gd.advance.clear();
}

void surge::atom::text::make_glyphs_resident(glyph_data &gd) {
  for (auto handle : gd.texture_handle) {
    glMakeTextureHandleResidentARB(handle);
  }
}

void surge::atom::text::make_glyphs_non_resident(glyph_data &gd) {
  for (auto handle : gd.texture_handle) {
    glMakeTextureHandleNonResidentARB(handle);
  }
}

void surge::atom::text::append_text_draw_data(text_draw_data &tdd, const glyph_data &gd,
                                              std::string_view text, glm::vec3 &&baseline_origin,
                                              glm::vec4 &&color) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::append_text_draw_data");
#endif

  glm::vec4 bbox{
      baseline_origin[0],
      baseline_origin[1],
      0.0f,
      0.0f,
  };

  auto pen_origin{baseline_origin};

  for (const auto &c : text) {
    // White space
    if (c == ' ') {
      pen_origin[0] += static_cast<float>(gd.whitespace_advance >> 6);
      bbox[2] += static_cast<float>(gd.whitespace_advance >> 6);
      continue;
    }

    // New line + carrige return
    if (c == '\n') {
      // TODO
      continue;
    }

    // Unsupported characters
    if (c < 33 || c > 126) {
      // TODO: print �
      continue;
    }

    // Printable characters
    const auto char_idx{static_cast<usize>(c) - 33};

    const auto bearing{glm::vec3{gd.bearing_x[char_idx], -gd.bearing_y[char_idx], 0.0f}};
    const auto glyph_origin{pen_origin + bearing};
    const auto glyph_scale{glm::vec3{gd.bitmap_width[char_idx], gd.bitmap_height[char_idx], 1.0f}};
    const auto glyph_model{glm::scale(glm::translate(glm::mat4{1.0f}, glyph_origin), glyph_scale)};

    tdd.glyph_models.push_back(glyph_model);
    tdd.texture_handles.push_back(gd.texture_handle[char_idx]);

    pen_origin[0] += static_cast<float>(gd.advance[char_idx] >> 6);

    // Compute bbox
    if (glyph_origin[1] < bbox[1]) {
      bbox[1] = glyph_origin[1];
    }

    bbox[2] += glyph_scale[0];

    if (glyph_scale[1] > bbox[3]) {
      bbox[3] = glyph_scale[1];
    }
  }

  tdd.color = color;
  tdd.bounding_box = bbox;
}

auto surge::atom::text::create_text_draw_data(const glyph_data &gd, std::string_view text,
                                              glm::vec3 &&baseline_origin,
                                              glm::vec4 &&color) noexcept -> text_draw_data {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::create_text_draw_data");
#endif

  text_draw_data tdd{};
  tdd.texture_handles.reserve(text.size());
  tdd.glyph_models.reserve(text.size());

  append_text_draw_data(tdd, gd, text, std::move(baseline_origin), std::move(color));

  return tdd;
}

void surge::atom::text::draw(const GLuint &sp, const sprite::buffer_data &bd, const glm::mat4 &proj,
                             const glm::mat4 &view, const text_draw_data &tdd) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::text::draw");
  TracyGpuZone("GPU surge::atom::text::draw");
#endif
  if (tdd.glyph_models.size() == 0 || tdd.texture_handles.size() == 0) {
    return;
  }

  glUseProgram(sp);

  renderer::uniforms::set(sp, "projection", proj);
  renderer::uniforms::set(sp, "view", view);

  renderer::uniforms::set(sp, "models", tdd.glyph_models.data(), tdd.glyph_models.size());
  renderer::uniforms::set(sp, "textures", tdd.texture_handles.data(), tdd.texture_handles.size());
  renderer::uniforms::set(sp, "color", tdd.color);

  glBindVertexArray(bd.VAO);
  glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr,
                          gsl::narrow_cast<GLsizei>(tdd.glyph_models.size()));
}