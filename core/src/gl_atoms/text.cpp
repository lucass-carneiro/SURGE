#include "gl_atoms/text.hpp"

#include "allocators.hpp"
#include "gl_atoms/shaders.hpp"
#include "logging.hpp"
#include "renderer.hpp"

// clang-format off
#include <freetype/freetype.h>
#include <freetype/ftsystem.h>
#include <freetype/ftmodapi.h>
// clang-format on

#include <array>
#include <gsl/gsl-lite.hpp>
#include <tl/expected.hpp>

// https://gist.github.com/jiaoyk/c9ba7fed2a086c73aecb3edee83af0f6

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

auto surge::gl_atom::text_engine::create() noexcept -> tl::expected<text_engine, error> {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::text_engine::create()");
#endif
  text_engine engine{};

  log_info("Creating FreeType library");
  auto status{FT_New_Library(&ft_mimalloc, &engine.ft_library)};

  if (status != 0) {
    log_error("Error creating FreeType library: %s", FT_Error_String(status));
    return tl::unexpected{error::freetype_init};
  }

  log_info("Adding FreeType modules");
  FT_Add_Default_Modules(engine.ft_library);

  return engine;
}

void surge::gl_atom::text_engine::destroy() noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::text_engine::destroy");
#endif

  for (auto &face : faces) {
    log_info("Unloading face %s", face.second->family_name);
    FT_Done_Face(face.second);
  }

  log_info("Destroying FreeType library");
  const auto status{FT_Done_Library(ft_library)};

  if (status != 0) {
    log_error("Error destroying FreeType library: %s", FT_Error_String(status));
  }
}

auto surge::gl_atom::text_engine::load_face(const char *path, std::string_view name,
                                            FT_F26Dot6 size_in_pts, FT_UInt resolution_dpi) noexcept
    -> std::optional<error> {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::text_engine::load_face()");
#endif
  log_info("Loading face %s with name %s", path, name.data());

  FT_Face face{};
  auto status{FT_New_Face(ft_library, path, 0, &face)};

  if (status != 0) {
    log_error("Error loading face %s: %s", path, FT_Error_String(status));
    return error::freetype_face_load;
  }

  status = FT_Set_Char_Size(face, 0, size_in_pts * 64, resolution_dpi, resolution_dpi);
  if (status != 0) {
    log_error("Unable to set face %s glyphs to size: %s", face->family_name,
              FT_Error_String(status));
    return error::freetype_set_face_size;
  }

  faces[name] = face;

  return {};
}

auto surge::gl_atom::text_engine::get_face(std::string_view name) noexcept
    -> std::optional<FT_Face> {
  if (faces.contains(name)) {
    return faces[name];
  } else {
    log_warn("The text engine has no face named %s", name.data());
    return {};
  }
}

auto surge::gl_atom::text_engine::get_faces() noexcept -> hash_map<std::string_view, FT_Face> & {
  return faces;
}

auto surge::gl_atom::glyph_cache::create(FT_Face face, language lang) noexcept
    -> tl::expected<glyph_cache, error> {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::glyph_cache::create()");
#endif
  if (!face) {
    log_error("Unable to create glyph cache: null font face passed");
    return tl::unexpected{error::freetype_null_face};
  }

  log_info("Creating glyph cache for %s", face->family_name);

  const auto status{FT_Select_Charmap(face, FT_ENCODING_UNICODE)};
  if (status != 0) {
    log_error("Unable to set charmap for face %s: %s", face->family_name, FT_Error_String(status));
    return tl::unexpected{error::freetype_charmap};
  }

  glyph_cache cache{};

  switch (lang) {
  case english: {
    // Printable ASCII
    for (FT_ULong c = 0x00000021; c <= 0x0000007e; c++) {
      const auto load_err{cache.load_character(face, c)};
      if (load_err) {
        return tl::unexpected{load_err.value()};
      }
    }

    // White space
    auto load_err{cache.load_nonprintable_character(face, 0x00000020)};
    if (load_err) {
      return tl::unexpected{load_err.value()};
    }

    // New line
    load_err = cache.load_nonprintable_character(face, 0x0000000a);
    if (load_err) {
      return tl::unexpected{load_err.value()};
    }

    // Tab
    load_err = cache.load_nonprintable_character(face, 0x0000000b);
    if (load_err) {
      return tl::unexpected{load_err.value()};
    }

    // Replacement character �
    load_err = cache.load_nonprintable_character(face, 0x0000FFFD);
    if (load_err) {
      return tl::unexpected{load_err.value()};
    }

    break;
  }

  default:
    break;
  }

  return cache;
}

void surge::gl_atom::glyph_cache::destroy() noexcept {
  log_info("Destroying glyph cache");

  make_non_resident();
  for (const auto &id : texture_ids) {
    if (id.second != 0) {
      glDeleteTextures(1, &(id.second));
    }
  }
}

auto surge::gl_atom::glyph_cache::load_character(FT_Face face, FT_ULong c) noexcept
    -> std::optional<error> {

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  const auto status{FT_Load_Char(face, c, FT_LOAD_RENDER)};
  if (status != 0) {
    log_error("Unable to load ASCII character %lu for face %s: %s", c, face->family_name,
              FT_Error_String(status));
    return error::freetype_character_load;
  } else {

    const auto bw{face->glyph->bitmap.width};
    const auto bh{face->glyph->bitmap.rows};

    bitmap_dims[c] = glm::vec<2, unsigned int>{bw, bh};
    bearings[c] = glm::vec<2, FT_Int>{face->glyph->bitmap_left, face->glyph->bitmap_top};
    advances[c] = glm::vec<2, FT_Pos>{face->glyph->advance.x, face->glyph->advance.y};

    GLuint texture{0};
    glCreateTextures(GL_TEXTURE_2D, 1, &texture);

    // Wrapping
    glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // Filtering
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLfloat max_aniso{0};
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_aniso);
    glTextureParameterf(texture, GL_TEXTURE_MAX_ANISOTROPY, max_aniso);

    constexpr const auto internal_format{GL_R8};
    constexpr const auto format{GL_RED};

    glTextureStorage2D(texture, 4, internal_format, static_cast<GLsizei>(bw),
                       static_cast<GLsizei>(bh));
    glTextureSubImage2D(texture, 0, 0, 0, static_cast<GLsizei>(bw), static_cast<GLsizei>(bh),
                        format, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);

    glGenerateTextureMipmap(texture);

    const auto handle{glGetTextureHandleARB(texture)};
    if (handle == 0) {
      log_error("Unable to create texture handle for character %lu", c);
      return error::texture_handle_creation;
    } else {
      texture_ids[c] = texture;
      texture_handles[c] = handle;
    }
  }

  return {};
}

auto surge::gl_atom::glyph_cache::load_nonprintable_character(FT_Face face, FT_ULong c) noexcept
    -> std::optional<error> {

  const auto status{FT_Load_Char(face, c, FT_LOAD_RENDER)};
  if (status != 0) {
    log_error("Unable to load ASCII character %lu for face %s: %s", c, face->family_name,
              FT_Error_String(status));
    return error::freetype_character_load;
  } else {

    const auto bw{face->glyph->bitmap.width};
    const auto bh{face->glyph->bitmap.rows};

    bitmap_dims[c] = glm::vec<2, unsigned int>{bw, bh};
    bearings[c] = glm::vec<2, FT_Int>{face->glyph->bitmap_left, face->glyph->bitmap_top};
    advances[c] = glm::vec<2, FT_Pos>{face->glyph->advance.x, face->glyph->advance.y};
    texture_ids[c] = 0;
    texture_handles[c] = 0;
    return {};
  }
}

void surge::gl_atom::glyph_cache::make_resident() noexcept {
  for (const auto &handle : texture_handles) {
    if (handle.second != 0 && !glIsTextureHandleResidentARB(handle.second)) {
      glMakeTextureHandleResidentARB(handle.second);
    }
  }
}

void surge::gl_atom::glyph_cache::make_non_resident() noexcept {
  for (const auto &handle : texture_handles) {
    if (handle.second != 0 && glIsTextureHandleResidentARB(handle.second)) {
      glMakeTextureHandleNonResidentARB(handle.second);
    }
  }
}

auto surge::gl_atom::glyph_cache::get_texture_handles() const noexcept
    -> const hash_map<FT_ULong, GLuint64> & {
  return texture_handles;
}

auto surge::gl_atom::glyph_cache::get_bitmap_dims() const noexcept
    -> const hash_map<FT_ULong, glm::vec<2, unsigned int>> & {
  return bitmap_dims;
}

auto surge::gl_atom::glyph_cache::get_bearings() const noexcept
    -> const hash_map<FT_ULong, glm::vec<2, FT_Int>> & {
  return bearings;
}

auto surge::gl_atom::glyph_cache::get_advances() const noexcept
    -> const hash_map<FT_ULong, glm::vec<2, FT_Pos>> & {
  return advances;
}

auto surge::gl_atom::text_buffer::create(usize max_chars) noexcept
    -> tl::expected<text_buffer, error> {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::text_buffer::create()");
  TracyGpuZone("GPU surge::gl_atom::text_buffer::create()");
#endif

  text_buffer tb{};

  /******************
   * Compile shader *
   ******************/
  const auto text_shader{shader::create_shader_program("shaders/gl/text.vert", "shaders/gl/text.frag")};
  if (!text_shader) {
    log_error("Unable to create text shader");
    return tl::unexpected{text_shader.error()};
  }

  tb.text_shader = *text_shader;

  /***************
   * Gen Buffers *
   ***************/
  glGenBuffers(1, &tb.VBO);
  glGenBuffers(1, &tb.EBO);
  glGenVertexArrays(1, &tb.VAO);

  /***************
   * Create quad *
   ***************/
  const std::array<float, 20> vertex_attributes{
      0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // bottom left
      1.0f, 1.0f, 0.0f, 1.0f, 1.0f, // bottom right
      1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // top right
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, // top left
  };

  const std::array<GLuint, 6> draw_indices{0, 1, 2, 2, 3, 0};

  glBindVertexArray(tb.VAO);

  glBindBuffer(GL_ARRAY_BUFFER, tb.VBO);
  glBufferData(GL_ARRAY_BUFFER, vertex_attributes.size() * sizeof(float), vertex_attributes.data(),
               GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tb.EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, draw_indices.size() * sizeof(GLuint), draw_indices.data(),
               GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        reinterpret_cast<const void *>(3 * sizeof(float))); // NOLINT

  /***************
   * Create GBAs *
   ***************/
  tb.models = gba<glm::mat4>::create(max_chars, "Text Modesl GBA");
  tb.texture_handles = gba<GLuint64>::create(max_chars, "Text Texture Handles GBA");

  return tb;
}

void surge::gl_atom::text_buffer::destroy() noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::text_buffer::destroy()");
  TracyGpuZone("GPU surge::gl_atom::text_buffer::destroy()");
#endif
  log_info("Destroying Text Buffer");

  texture_handles.destroy();
  models.destroy();
  glDeleteBuffers(1, &(EBO));
  glDeleteBuffers(1, &(VBO));
  glDeleteVertexArrays(1, &(VAO));

  shader::destroy_shader_program(text_shader);
}

auto surge::gl_atom::text_buffer::get_bbox_size(glyph_cache &cache, std::string_view text) noexcept
    -> glm::vec2 {
  float bb_w{0.0f};
  float bb_h{0.0f};

  // TODO: Iterate over UTF-32 codepoints
  for (const auto &c : text) {
    auto cdpnt{static_cast<FT_ULong>(c)};

    // Unrecognized character
    if (!cache.get_texture_handles().contains(cdpnt)) {
      cdpnt = 0x0000FFFD;
    }

    const auto bitmap_dim{cache.get_bitmap_dims().at(cdpnt)};
    const auto &advance{cache.get_advances().at(cdpnt)};

    const auto glyph_w{static_cast<float>(advance[0] >> 6)};
    const auto glyph_h{static_cast<float>(bitmap_dim[1])};

    if (bb_w + glyph_w > bb_w) {
      bb_w += glyph_w;
    }

    if (glyph_h > bb_h) {
      bb_h = glyph_h;
    }
  }

  return glm::vec2{bb_w, bb_h};
}

void surge::gl_atom::text_buffer::push(const glm::vec3 &baseline_origin, const glm::vec2 &scale,
                                       glyph_cache &cache, std::string_view text) noexcept {

  auto pen_origin{baseline_origin};

  // TODO: Iterate over UTF-32 codepoints
  for (const auto &c : text) {
    auto cdpnt{static_cast<FT_ULong>(c)};

    // Unrecognized character
    if (!cache.get_texture_handles().contains(cdpnt)) {
      cdpnt = 0x0000FFFD;
    }

    const auto &texture_handle{cache.get_texture_handles().at(cdpnt)};
    const auto &bitmap_dim{cache.get_bitmap_dims().at(cdpnt)};
    const auto &bearing{cache.get_bearings().at(cdpnt)};
    const auto &advance{cache.get_advances().at(cdpnt)};

    // \n
    if (cdpnt == 0x0000000a) {
      pen_origin[0] = baseline_origin[0];
      pen_origin[1] += static_cast<float>(advance[1] >> 6) * scale[1];
      continue;
    }

    // Printable characters
    const auto glyph_origin{pen_origin
                            + glm::vec3{static_cast<float>(bearing[0]) * scale[0],
                                        -static_cast<float>(bearing[1]) * scale[1], 0.0f}};

    const glm::vec3 glyph_scale{static_cast<float>(bitmap_dim[0]) * scale[0],
                                static_cast<float>(bitmap_dim[1]) * scale[1], 1.0f};

    const auto glyph_model{glm::scale(glm::translate(glm::mat4{1.0f}, glyph_origin), glyph_scale)};

    models.push(glyph_model);
    texture_handles.push(texture_handle);

    pen_origin[0] += static_cast<float>(advance[0] >> 6) * scale[0];
  }
}

void surge::gl_atom::text_buffer::push_centered(const glm::vec3 &baseline_origin,
                                                float intended_scale, const glm::vec2 &region_dims,
                                                glyph_cache &cache, std::string_view text,
                                                float decrement_step) noexcept {

  const auto bbox{get_bbox_size(cache, text)};

  auto scale{intended_scale};
  auto scaled_bbox{scale * bbox};

  while (scaled_bbox[0] > region_dims[0]) {
    scale -= decrement_step;
    scaled_bbox = scale * bbox;
  }

  const glm::vec3 shift{(region_dims[0] - scaled_bbox[0]) / 2.0f,
                        -(region_dims[1] - scaled_bbox[1]) / 2.0f, 0.0f};

  push(baseline_origin + shift, glm::vec2{scale}, cache, text);
}

void surge::gl_atom::text_buffer::reset() noexcept {
  models.reset();
  texture_handles.reset();
}

void surge::gl_atom::text_buffer::draw(const glm::vec4 &color) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::text_buffer::draw");
  TracyGpuZone("GPU surge::gl_atom::text_buffer::draw");
#endif

  if (texture_handles.size() != 0 && models.size() != 0) {
    models.bind(GL_SHADER_STORAGE_BUFFER, 3);
    texture_handles.bind(GL_SHADER_STORAGE_BUFFER, 4);

    glUseProgram(text_shader);

    glUniform4fv(5, 1, glm::value_ptr(color));

    glBindVertexArray(VAO);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr,
                            gsl::narrow_cast<GLsizei>(texture_handles.size()));

    models.lock_write_buffer();
    texture_handles.lock_write_buffer();
  }
}