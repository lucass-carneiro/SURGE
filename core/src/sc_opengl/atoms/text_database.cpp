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
  const bool size_equals{ci.fonts.size() == ci.sizes_in_pts.size()
                         && ci.fonts.size() == ci.resolution_dpis.size()
                         && ci.fonts.size() == ci.langs.size()};

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
      load_err = load_nonprintable_character(cd, face, ci.replacement_char_code);
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