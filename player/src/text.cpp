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

auto surge::atom::text::create_buffers(usize max_chars) noexcept -> buffer_data {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::text::create_buffers");
  TracyGpuZone("GPU surge::atom::text::create_buffers");
#endif

  /***************
   * Gen Buffers *
   ***************/
  log_info("Creating text buffers");

  GLuint VAO{0};
  GLuint VBO{0};
  GLuint EBO{0};

  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);
  glGenVertexArrays(1, &VAO);

  /***************
   * Create quad *
   ***************/
  log_info("Creating text base quad");

  const std::array<float, 20> vertex_attributes{
      0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // bottom left
      1.0f, 1.0f, 0.0f, 1.0f, 1.0f, // bottom right
      1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // top right
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, // top left
  };

  const std::array<GLuint, 6> draw_indices{0, 1, 2, 2, 3, 0};

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, vertex_attributes.size() * sizeof(float), vertex_attributes.data(),
               GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, draw_indices.size() * sizeof(GLuint), draw_indices.data(),
               GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        reinterpret_cast<const void *>(3 * sizeof(float))); // NOLINT

  /****************
   * Create SSBOs *
   ****************/
  log_info("Creating text model matrices buffer");
  GLuint MMB{0};
  glCreateBuffers(1, &MMB);
  glNamedBufferStorage(MMB, static_cast<GLsizeiptr>(sizeof(glm::mat4) * max_chars), nullptr,
                       GL_DYNAMIC_STORAGE_BIT);

  log_info("Creating sprite texture buffer");
  GLuint THB{0};
  glCreateBuffers(1, &THB);
  glNamedBufferStorage(THB, static_cast<GLsizeiptr>(sizeof(GLuint64) * max_chars), nullptr,
                       GL_DYNAMIC_STORAGE_BIT);

  return buffer_data{VBO, EBO, VAO, MMB, THB};
}

void surge::atom::text::destroy_buffers(const buffer_data &bd) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::text::destroy_buffers");
  TracyGpuZone("GPU surge::atom::text::destroy_buffers");
#endif

  log_info("Deleting text buffer data\n  VBO: %u\n  EBO: %u\n  VAO: %u\n  MMB: %u\n  THB: %u",
           bd.VBO, bd.EBO, bd.VAO, bd.MMB, bd.THB);

  glDeleteBuffers(1, &(bd.VBO));
  glDeleteBuffers(1, &(bd.EBO));
  glDeleteVertexArrays(1, &(bd.VAO));

  glDeleteBuffers(1, &(bd.MMB));
  glDeleteBuffers(1, &(bd.THB));
}

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
  ZoneScopedN("surge::atom::text::load_face()");
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

auto surge::atom::text::load_glyphs(FT_Library, FT_Face face, FT_F26Dot6 size_in_pts,
                                    FT_UInt resolution_dpi) noexcept
    -> tl::expected<glyph_data, error> {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::text::load_glyphs()");
  TracyGpuZone("GPU surge::atom::text::load_glyphs()");
#endif

  log_info("Loading glyphs metrics in %s", face->family_name);

  auto status{FT_Set_Char_Size(face, 0, size_in_pts * 64, resolution_dpi, resolution_dpi)};
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
    gd.line_spacing = face->size->metrics.height;
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
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

      // Filtering
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      GLfloat max_aniso{0};
      glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_aniso);
      glTextureParameterf(texture, GL_TEXTURE_MAX_ANISOTROPY, max_aniso);

      constexpr const auto internal_format{GL_R8};
      constexpr const auto format{GL_RED};

      glTextureStorage2D(texture, 1, internal_format, static_cast<GLsizei>(bw),
                         static_cast<GLsizei>(bh));
      glTextureSubImage2D(texture, 0, 0, 0, static_cast<GLsizei>(bw), static_cast<GLsizei>(bh),
                          format, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);

      glGenerateTextureMipmap(texture);

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
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::text::unload_glyphs");
  TracyGpuZone("GPU surge::atom::text::unload_glyphs");
#endif

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
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::text::make_glyphs_resident(vector)");
  TracyGpuZone("GPU surge::atom::text::make_glyphs_resident(vector)");
#endif

  for (auto handle : gd.texture_handle) {
    glMakeTextureHandleResidentARB(handle);
  }
}

void surge::atom::text::make_glyphs_non_resident(glyph_data &gd) {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::text::make_glyphs_non_resident(vector)");
  TracyGpuZone("GPU surge::atom::text::make_glyphs_non_resident(vector)");
#endif

  for (auto handle : gd.texture_handle) {
    glMakeTextureHandleNonResidentARB(handle);
  }
}

void surge::atom::text::send_buffers(const buffer_data &bd, const text_draw_data &tdd) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::text::send_buffers");
  TracyGpuZone("GPU surge::atom::text::send_buffers");
#endif

  if (tdd.texture_handles.size() != 0 && tdd.glyph_models.size() != 0) {
    glNamedBufferSubData(bd.MMB, 0,
                         static_cast<GLsizeiptr>(sizeof(glm::mat4) * tdd.glyph_models.size()),
                         tdd.glyph_models.data());

    glNamedBufferSubData(bd.THB, 0,
                         static_cast<GLsizeiptr>(sizeof(GLuint64) * tdd.texture_handles.size()),
                         tdd.texture_handles.data());
  }
}

void surge::atom::text::append_text_draw_data(text_draw_data &tdd, const glyph_data &gd,
                                              std::string_view text,
                                              const glm::vec3 &baseline_origin,
                                              const glm::vec4 &color,
                                              const glm::vec2 &scale) noexcept {
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
      pen_origin[0] += static_cast<float>(gd.whitespace_advance >> 6) * scale[0];
      bbox[2] += static_cast<float>(gd.whitespace_advance >> 6) * scale[0];
      continue;
    }

    // New line + carrige return
    if (c == '\n') {
      pen_origin[0] = baseline_origin[0];
      pen_origin[1] += static_cast<float>(gd.line_spacing >> 6) * scale[1];
      bbox[3] += static_cast<float>(gd.line_spacing >> 6) * scale[1];
      continue;
    }

    // Unsupported characters
    if (c < 33 || c > 126) {
      log_warn("Character %c is unsupported", c);
      continue;
    }

    // Printable characters
    const auto char_idx{static_cast<usize>(c) - 33};

    const glm::vec3 bearing{static_cast<float>(gd.bearing_x[char_idx]) * scale[0],
                            static_cast<float>(-gd.bearing_y[char_idx]) * scale[1], 0.0f};

    const auto glyph_origin{pen_origin + bearing};

    const glm::vec3 glyph_scale{static_cast<float>(gd.bitmap_width[char_idx]) * scale[0],
                                static_cast<float>(gd.bitmap_height[char_idx]) * scale[1], 1.0f};

    const auto glyph_model{glm::scale(glm::translate(glm::mat4{1.0f}, glyph_origin), glyph_scale)};

    tdd.glyph_models.push_back(glyph_model);
    tdd.texture_handles.push_back(gd.texture_handle[char_idx]);

    pen_origin[0] += static_cast<float>(gd.advance[char_idx] >> 6) * scale[0];

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

void surge::atom::text::draw(const GLuint &sp, const buffer_data &bd, const GLuint &MPSB,
                             const text_draw_data &tdd) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::text::draw");
  TracyGpuZone("GPU surge::atom::text::draw");
#endif

  if (tdd.texture_handles.size() != 0 && tdd.glyph_models.size() != 0) {

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, MPSB);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, bd.MMB);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, bd.THB);

    glUseProgram(sp);

    renderer::uniforms::set(sp, "color", tdd.color);

    glBindVertexArray(bd.VAO);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr,
                            gsl::narrow_cast<GLsizei>(tdd.glyph_models.size()));
  }
}