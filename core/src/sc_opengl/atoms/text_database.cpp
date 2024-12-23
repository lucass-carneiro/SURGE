#include "sc_opengl/atoms/text_database.hpp"

#include "sc_allocators.hpp"
#include "sc_container_types.hpp"
#include "sc_glm_includes.hpp"
#include "sc_logging.hpp"
#include "sc_opengl/atoms/shaders.hpp"
#include "sc_options.hpp"

// clang-format off
#include <freetype/freetype.h>
#include <freetype/ftsystem.h>
#include <freetype/ftmodapi.h>
// clang-format on

#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#  include <tracy/TracyOpenGL.hpp>
#endif

struct cache_data {
  surge::hash_map<FT_ULong, GLuint> texture_ids{};
  surge::hash_map<FT_ULong, GLuint64> texture_handles{};

  surge::hash_map<FT_ULong, glm::vec<2, unsigned int>> bitmap_dims{};

  surge::hash_map<FT_ULong, glm::vec<2, FT_Int>> bearings{};
  surge::hash_map<FT_ULong, glm::vec<2, FT_Pos>> advances{};
};

struct surge::gl_atom::text_database::glyph_cache::cache_t {
  vector<cache_data> caches{};
};

static auto FT_malloc(FT_Memory, long size) -> void * {
  return surge::allocators::mimalloc::malloc(static_cast<surge::usize>(size));
}

static void FT_free(FT_Memory, void *block) { surge::allocators::mimalloc::free(block); }

static auto FT_realloc(FT_Memory, long, long new_size, void *block) -> void * {
  return surge::allocators::mimalloc::realloc(block, static_cast<surge::usize>(new_size));
}

static auto load_character(cache_data &cd, FT_Face face, FT_ULong c) noexcept
    -> std::optional<surge::error> {
  using namespace surge;

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  const auto status{FT_Load_Char(face, c, FT_LOAD_RENDER)};
  if (status != 0) {
    log_error("Unable to load ASCII character {} for face {}: {}", c, face->family_name,
              FT_Error_String(status));
    return error::freetype_character_load;
  } else {

    const auto bw{face->glyph->bitmap.width};
    const auto bh{face->glyph->bitmap.rows};

    cd.bitmap_dims[c] = glm::vec<2, unsigned int>{bw, bh};
    cd.bearings[c] = glm::vec<2, FT_Int>{face->glyph->bitmap_left, face->glyph->bitmap_top};
    cd.advances[c] = glm::vec<2, FT_Pos>{face->glyph->advance.x, face->glyph->advance.y};

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
      log_error("Unable to create texture handle for character {}", c);
      return error::texture_handle_creation;
    } else {
      cd.texture_ids[c] = texture;
      cd.texture_handles[c] = handle;
    }
  }

  return {};
}

auto load_nonprintable_character(cache_data &cd, FT_Face face, FT_ULong c) noexcept
    -> std::optional<surge::error> {
  using namespace surge;

  const auto status{FT_Load_Char(face, c, FT_LOAD_RENDER)};
  if (status != 0) {
    log_error("Unable to load character {} for face {}: {}", c, face->family_name,
              FT_Error_String(status));
    return error::freetype_character_load;
  } else {

    const auto bw{face->glyph->bitmap.width};
    const auto bh{face->glyph->bitmap.rows};

    cd.bitmap_dims[c] = glm::vec<2, unsigned int>{bw, bh};
    cd.bearings[c] = glm::vec<2, FT_Int>{face->glyph->bitmap_left, face->glyph->bitmap_top};
    cd.advances[c] = glm::vec<2, FT_Pos>{face->glyph->advance.x, face->glyph->advance.y};
    cd.texture_ids[c] = 0;
    cd.texture_handles[c] = 0;

    return {};
  }
}

auto surge::gl_atom::text_database::glyph_cache::create(create_info ci)
    -> tl::expected<cache, error> {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::text_database::glyph_cache::create");
  TracyGpuZone("GPU surge::gl_atom::text_database::glyph_cache::create");
#endif

  // Read creation data
  const bool size_equals{
      ci.fonts.size() == ci.sizes_in_pts.size() && ci.fonts.size() == ci.resolution_dpis.size()
      && ci.fonts.size() == ci.langs.size() && ci.fonts.size() == ci.replacement_char_codes.size()

  };

  if (!size_equals) {
    log_error("Glyph cache creation failed due to inconsistent data. The number of fonts, sizes, "
              "resolutions and languages passed on creation must be equal");
    return tl::unexpected{gc_inconsistent_creation_size};
  }

  const auto num_fonts{ci.fonts.size()};

  // Alloc instance
  auto gc{static_cast<cache>(allocators::mimalloc::malloc(sizeof(cache_t)))};

  if (gc == nullptr) {
    log_error("Unable to allocate glyph cache instance");
    return tl::unexpected{gc_instance_alloc};
  }

  new (gc)(cache_t)();

  // Alloc memory for holding the caches
  gc->caches.reserve(num_fonts);

  // Create FreeType instance
  FT_MemoryRec_ ft_mimalloc{nullptr, FT_malloc, FT_free, FT_realloc};
  FT_Library ft_library{nullptr};

  log_info("Creating FreeType library");
  auto status{FT_New_Library(&ft_mimalloc, &ft_library)};

  if (status != 0) {
    log_error("Error creating FreeType library: {}", FT_Error_String(status));
    return tl::unexpected{error::freetype_init};
  }

  log_info("Adding FreeType modules");
  FT_Add_Default_Modules(ft_library);

  // Create font faces
  vector<FT_Face> faces{};
  faces.reserve(num_fonts);

  for (usize i = 0; i < num_fonts; i++) {
    const auto &path{ci.fonts[i]};
    const auto &size_in_pts{ci.sizes_in_pts[i]};
    const auto &resolution_dpi{ci.resolution_dpis[i]};

    log_info("Loading face {}", path);

    FT_Face face{};
    auto status{FT_New_Face(ft_library, path, 0, &face)};

    if (status != 0) {
      log_error("Error loading face {}: {}", path, FT_Error_String(status));
      return tl::unexpected{error::freetype_face_load};
    }

    status = FT_Set_Char_Size(face, 0, size_in_pts * 64, resolution_dpi, resolution_dpi);
    if (status != 0) {
      log_error("Unable to set face {} glyphs to size: {}", path, FT_Error_String(status));
      return tl::unexpected{error::freetype_set_face_size};
    }

    faces.push_back(face);
  }

  for (usize i = 0; i < num_fonts; i++) {
    const auto &path{ci.fonts[i]};
    const auto &face{faces[i]};
    const auto &lang{ci.langs[i]};
    const auto &replacement_char_code{ci.replacement_char_codes[i]};

    log_info("Creating glyph cache for font {} {}", path, static_cast<void *>(face));

    const auto status{FT_Select_Charmap(face, FT_ENCODING_UNICODE)};
    if (status != 0) {
      log_error("Unable to set charmap for font {}: {}", path, FT_Error_String(status));
      return tl::unexpected{error::freetype_charmap};
    }

    cache_data cd{};

    switch (lang) {
    case languages::en_US: {

      // Printable ASCII
      for (FT_ULong c = 0x00000021; c <= 0x0000007e; c++) {
        const auto load_err{load_character(cd, face, c)};
        if (load_err) {
          return tl::unexpected{load_err.value()};
        }
      }

      // White space
      auto load_err{load_nonprintable_character(cd, face, 0x00000020)};
      if (load_err) {
        return tl::unexpected{load_err.value()};
      }

      // New line
      load_err = load_nonprintable_character(cd, face, 0x0000000a);
      if (load_err) {
        return tl::unexpected{load_err.value()};
      }

      // Tab
      load_err = load_nonprintable_character(cd, face, 0x0000000b);
      if (load_err) {
        return tl::unexpected{load_err.value()};
      }

      // Replacement character
      load_err = load_nonprintable_character(cd, face, replacement_char_code);
      if (load_err) {
        return tl::unexpected{load_err.value()};
      }

      break;
    }

      // TODO: pt_BR

    default:
      break;
    }

    gc->caches.push_back(cd);
  }

  // Destroy font faces
  for (usize i = 0; i < num_fonts; i++) {
    const auto &font{ci.fonts[i]};
    const auto &face{faces[i]};

    log_info("Unloading face {}", font);
    FT_Done_Face(face);
  }

  // Destroy FreeType instance
  log_info("Destroying FreeType library");
  status = FT_Done_Library(ft_library);

  if (status != 0) {
    log_error("Error destroying FreeType library: {}", FT_Error_String(status));
  }

  // Optionally make all loaded glyph textures resident
  if (ci.make_all_resident) {
    reside_all(gc);
  }

  // Done
  log_info("Created new text database, handle {}", static_cast<void *>(gc));

  return gc;
}

void surge::gl_atom::text_database::glyph_cache::destroy(cache gc) {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::text_database::glyph_cache::destroy");
  TracyGpuZone("GPU surge::gl_atom::text_database::glyph_cache::destroy");
#endif

  log_info("Destroying text database, handle {}", static_cast<void *>(gc));

  // Make all glyph textures non-resident
  unreside_all(gc);

  // Destroy all glyph textures
  for (const auto &c : gc->caches) {
    for (const auto &id : c.texture_ids) {
      if (id.second != 0) {
        glDeleteTextures(1, &(id.second));
      }
    }
  }

  // Free glyph cache vector
  gc->caches.~vector();

  // Free instance
  allocators::mimalloc::free(static_cast<void *>(gc));
}

void surge::gl_atom::text_database::glyph_cache::reside_all(cache gc) {
  for (const auto &c : gc->caches) {
    for (const auto &handle : c.texture_handles) {
      if (handle.second != 0 && !glIsTextureHandleResidentARB(handle.second)) {
        glMakeTextureHandleResidentARB(handle.second);
      }
    }
  }
}

void surge::gl_atom::text_database::glyph_cache::unreside_all(cache gc) {
  for (const auto &c : gc->caches) {
    for (const auto &handle : c.texture_handles) {
      if (handle.second != 0 && glIsTextureHandleResidentARB(handle.second)) {
        glMakeTextureHandleNonResidentARB(handle.second);
      }
    }
  }
}

void surge::gl_atom::text_database::glyph_cache::reside(cache gc, usize index) {
  if (index < gc->caches.size()) {
    for (const auto &handle : gc->caches[index].texture_handles) {
      if (handle.second != 0 && !glIsTextureHandleResidentARB(handle.second)) {
        glMakeTextureHandleResidentARB(handle.second);
      }
    }
  } else {
    log_warn(
        "Unable to make font index {} in glyph cache {} non resident. The index does not exist",
        index, static_cast<void *>(gc));
  }
}

void surge::gl_atom::text_database::glyph_cache::unreside(cache gc, usize index) {
  if (index < gc->caches.size()) {
    for (const auto &handle : gc->caches[index].texture_handles) {
      if (handle.second != 0 && glIsTextureHandleResidentARB(handle.second)) {
        glMakeTextureHandleNonResidentARB(handle.second);
      }
    }
  } else {
    log_warn(
        "Unable to make font index {} in glyph cache {} non resident. The index does not exist",
        index, static_cast<void *>(gc));
  }
}

/* This struct needs to be aligned such that
 * it is a multiple of `alignment`, where
 *
 * GLint alignment{0};
 * glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &alignment);
 */
struct text_info {
  GLuint64 texture_handle{0};
  float color_mod[4]{1.0f, 1.0f, 1.0f, 1.0f};
  float model[16]{
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
  };
};

struct surge::gl_atom::text_database::text_buffer::buffer_t {
  usize max_glyphs{0};

  GLsync fence{nullptr};

  GLuint buffer_id{0};
  text_info *buffer_data{nullptr};

  usize write_idx{0};

  GLuint text_shader{0};

  GLuint VBO{0};
  GLuint EBO{0};
  GLuint VAO{0};
};

auto surge::gl_atom::text_database::text_buffer::create(create_info ci) noexcept
    -> tl::expected<buffer, error> {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::text_database::text_buffer::create");
  TracyGpuZone("GPU surge::gl_atom::text_database::text_buffer::create");
#endif

  // Alloc instance
  auto txtb{static_cast<buffer>(allocators::mimalloc::malloc(sizeof(buffer_t)))};

  if (txtb == nullptr) {
    log_error("Unable to allocate sprite database instance");
    return tl::unexpected{sdb_instance_alloc};
  }

  new (txtb)(buffer_t)();

  // Read create info
  txtb->max_glyphs = ci.max_glyphs;

  // Alloc GPU buffer
  const auto total_buffer_size{static_cast<GLsizeiptr>(sizeof(text_info) * txtb->max_glyphs)};

  constexpr GLbitfield map_flags{GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT};
  constexpr GLbitfield create_flags{map_flags | GL_DYNAMIC_STORAGE_BIT};

  glCreateBuffers(1, &(txtb->buffer_id));
  glNamedBufferStorage(txtb->buffer_id, total_buffer_size, nullptr, create_flags);
  txtb->buffer_data = static_cast<text_info *>(
      glMapNamedBufferRange(txtb->buffer_id, 0, total_buffer_size, map_flags));

  // Compile shaders
  const auto text_shader{shader::create_shader_program("shaders/gl/text_database.vert",
                                                       "shaders/gl/text_database.frag")};
  if (!text_shader) {
    log_error("Unable to create text_database shader");
    return tl::unexpected{text_shader.error()};
  }

  txtb->text_shader = *text_shader;

  // Vertex buffers
  glCreateVertexArrays(1, &(txtb->VAO));
  glCreateBuffers(1, &(txtb->VBO));
  glCreateBuffers(1, &(txtb->EBO));

  const std::array<float, 20> vertex_attributes{
      0.0f, 1.0f, 0.0f, 0.0f, 0.0f, // bottom left
      1.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom right
      1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top right
      0.0f, 0.0f, 0.0f, 0.0f, 1.0f, // top left
  };

  const std::array<GLuint, 6> draw_indices{0, 1, 2, 2, 3, 0};

  glBindVertexArray(txtb->VAO);

  glBindBuffer(GL_ARRAY_BUFFER, txtb->VBO);
  glBufferData(GL_ARRAY_BUFFER, vertex_attributes.size() * sizeof(float), vertex_attributes.data(),
               GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, txtb->EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, draw_indices.size() * sizeof(GLuint), draw_indices.data(),
               GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        reinterpret_cast<const void *>(3 * sizeof(float))); // NOLINT

  // Done
  log_info("Created new text buffer, handle {} using {} B of video memory",
           static_cast<void *>(txtb), total_buffer_size);

  return txtb;
}

void surge::gl_atom::text_database::text_buffer::destroy(buffer txtb) noexcept {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::text_database::text_buffer::destroy");
  TracyGpuZone("GPU surge::gl_atom::text_database::text_buffer::destroy");
#endif

  log_info("Destroying text buffer, handle {}", static_cast<void *>(txtb));

  wait_idle(txtb);

  // Delete vertex buffers
  glDeleteBuffers(1, &(txtb->EBO));
  glDeleteBuffers(1, &(txtb->VBO));
  glDeleteVertexArrays(1, &(txtb->VAO));

  // Delete shader program
  shader::destroy_shader_program(txtb->text_shader);

  // Free GPU buffer
  glUnmapNamedBuffer(txtb->buffer_id);
  glDeleteBuffers(1, &(txtb->buffer_id));

  // Free instance
  allocators::mimalloc::free(static_cast<void *>(txtb));
}

static void wait_buffer(surge::gl_atom::text_database::text_buffer::buffer txtb) {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::text_database::text_buffer::wait_buffer");
  TracyGpuZone("GPU surge::gl_atom::text_database::text_buffer::wait_buffer");
#endif

  auto &fence{txtb->fence};

  if (fence != nullptr) {
    while (true) {
      const auto wait_res{glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 1)};
      if (wait_res == GL_ALREADY_SIGNALED || wait_res == GL_CONDITION_SATISFIED) {
        glDeleteSync(fence);
        fence = nullptr;
        return;
      }
    }
  }
}

static void lock_buffer(surge::gl_atom::text_database::text_buffer::buffer txtb) {
  auto &fence{txtb->fence};

  if (fence == nullptr) {
    fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
  }
}

void surge::gl_atom::text_database::text_buffer::wait_idle(buffer txtb) noexcept {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::text_database::text_buffer::wait_idle");
  TracyGpuZone("GPU surge::gl_atom::text_database::text_buffer::wait_idle");
#endif

  wait_buffer(txtb);
}