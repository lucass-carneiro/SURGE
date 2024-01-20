#include "text.hpp"

#include "allocators.hpp"
#include "logging.hpp"

// clang-format off
#include <freetype/freetype.h>
#include <freetype/ftsystem.h>
#include <freetype/ftmodapi.h>
// clang-format on

// #include <algorithm>
#include <gsl/gsl-lite.hpp>
#include <tl/expected.hpp>

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#  include <tracy/TracyOpenGL.hpp>
#endif

static auto FT_malloc(FT_Memory, long size) noexcept -> void * {
  return surge::allocators::mimalloc::malloc(size);
}

static void FT_free(FT_Memory, void *block) noexcept { surge::allocators::mimalloc::free(block); }

static auto FT_realloc(FT_Memory, long, long new_size, void *block) noexcept -> void * {
  return surge::allocators::mimalloc::realloc(block, new_size);
}

static FT_MemoryRec_ ft_mimalloc{nullptr, FT_malloc, FT_free, FT_realloc};

auto surge::atom::text::create(const font_name_vec_t &fonts) noexcept
    -> tl::expected<buffer_data, error> {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::text::create");
  TracyGpuZone("GPU surge::atom::text::create");
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

  log_info("Allocating face vector");
  face_vec_t face_vec{};
  face_vec.reserve(fonts.size());

  log_info("Loading faces");
  for (const auto &name : fonts) {
    FT_Face face{};
    const auto status{FT_New_Face(lib, name.c_str(), 0, &face)};

    if (status != 0) {
      log_error("Error loading face %s: %s", name.c_str(), FT_Error_String(status));
      FT_Done_Library(lib);
      return tl::unexpected{error::freetype_face_not_found};
    } else
      face_vec.push_back(face);
  }

  log_info("Creating text vertex buffers");
  GLuint VAO{0}, VBO{0};
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);

  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);

  log_info("Font cache initialized");
  return buffer_data{lib, face_vec, VAO, VBO};
}

void surge::atom::text::terminate(buffer_data &data) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::text::terminate");
#endif

  log_info("Closing faces");
  for (const auto &face : data.faces) {
    FT_Done_Face(face);
  }

  log_info("Terminating FreeType library");
  const auto status{FT_Done_Library(data.library)};
  if (status != 0) {
    log_error("Unable to terminate FreeType library: %s", FT_Error_String(status));
  }
}

/* We will only save the ASCII characters ranging from codes char_start to char_end
 * see https://en.cppreference.com/w/cpp/language/ascii
 *
 * Each array in the charcater map will store (char_end - char_start) character data for each face
 * loaded.
 */
static constexpr const FT_ULong char_start{0};
static constexpr const FT_ULong char_end{126};
static constexpr const auto chars_per_face{char_end - char_start};

auto surge::atom::text::create_charmap(buffer_data &data, FT_UInt pixel_height,
                                       renderer::texture_filtering filtering) noexcept
    -> tl::expected<charmap_data, error> {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::text::create_charmap");
  TracyGpuZone("GPU surge::atom::text::create_charmap");
#endif

  const auto charmap_sizes{chars_per_face * data.faces.size()};

  log_info("Allocating memory for character maps");
  charmap_data map;
  map.chars_per_face = chars_per_face;
  map.texture_ids.reserve(charmap_sizes);
  map.sizes_x.reserve(charmap_sizes);
  map.sizes_y.reserve(charmap_sizes);
  map.bearings_x.reserve(charmap_sizes);
  map.bearings_y.reserve(charmap_sizes);
  map.advances.reserve(charmap_sizes);

  // Loop over faces
  for (auto &face : data.faces) {
    log_info("Face name: %s", face->family_name);

    log_info("  Setting size");
    auto status{FT_Set_Pixel_Sizes(face, 0, pixel_height)};
    if (status != 0) {
      log_error("Unable to set face %s size: %s", face->family_name, FT_Error_String(status));
      return tl::unexpected{error::freetype_set_face_size};
    }

    // Loop over characters
    log_info("  Loading characters");
    for (auto c = char_start; c <= char_end; c++) {

      status = FT_Load_Char(face, c, FT_LOAD_RENDER);
      if (status != 0) {
        log_error("Unable to load character %c for face %s: %s", static_cast<char>(c),
                  face->family_name, FT_Error_String(status));
        return tl::unexpected{error::freetype_character_load};
      }

      // Disable OpenGL byte-alignment restriction
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

      // Generate texture
      GLuint texture{0};
      glGenTextures(1, &texture);
      glBindTexture(GL_TEXTURE_2D, texture);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, gsl::narrow_cast<GLsizei>(face->glyph->bitmap.width),
                   gsl::narrow_cast<GLsizei>(face->glyph->bitmap.rows), 0, GL_RED, GL_UNSIGNED_BYTE,
                   face->glyph->bitmap.buffer);

      /// Set texture options
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLenum>(filtering));
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLenum>(filtering));

      // Restore unpack alignment
      glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

      // Store
      map.texture_ids.push_back(texture);
      map.sizes_x.push_back(face->glyph->bitmap.width);
      map.sizes_y.push_back(face->glyph->bitmap.rows);
      map.bearings_x.push_back(face->glyph->bitmap_left);
      map.bearings_y.push_back(face->glyph->bitmap_top);
      map.advances.push_back(face->glyph->advance.x);
    }
  }

  return map;
}

void surge::atom::text::destroy_charmap(const charmap_data &charmap) noexcept {
  for (auto map : charmap.texture_ids) {
    glDeleteTextures(1, &map);
  }
}

void surge::atom::text::draw(GLuint shader_program, const buffer_data &bd, const charmap_data &cd,
                             const draw_data &dd, std::string_view text,
                             float extra_vskip) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::text::draw");
  TracyGpuZone("GPU surge::atom::text::draw");
#endif

  // Set OpenGL state
  glUseProgram(shader_program);

  renderer::uniforms::set(shader_program, "projection", dd.projection);
  renderer::uniforms::set(shader_program, "text_color", dd.color);
  glActiveTexture(GL_TEXTURE0);

  glBindVertexArray(bd.VAO);

  float x{dd.position[0]};
  float y{dd.position[1]};

  for (const auto &c : text) {

    // Skip null character
    if (c == '\0') {
      continue;
    }

    const auto char_idx{static_cast<std::size_t>(c - char_start)
                        + dd.face_idx * (cd.chars_per_face + 1)};

    const float size_y{gsl::narrow_cast<float>(cd.sizes_y[char_idx])};

    // Render newline
    if (c == '\n') {
      x = dd.position[0];
      y += (size_y + extra_vskip) * dd.scale;
      continue;
    }

    // Skip rendering whitespace
    if (c == ' ' || c == '\t') {
      x += gsl::narrow_cast<float>(cd.advances[char_idx] >> 6) * dd.scale;
      continue;
    }

    const float size_x{gsl::narrow_cast<float>(cd.sizes_x[char_idx])};

    const float bearing_x{gsl::narrow_cast<float>(cd.bearings_x[char_idx])};
    const float bearing_y{gsl::narrow_cast<float>(cd.bearings_y[char_idx])};

    const float xpos{x + bearing_x * dd.scale};

    /* The user specifies the baseline position so the glyphs need to be shifted up by their size,
     * otherwise they would align their top to their baseline instead of their bottoms */
    const float ypos{(y - size_y * dd.scale) - (size_y - bearing_y) * dd.scale};

    const float w{size_x * dd.scale};
    const float h{size_y * dd.scale};

    // Update VBO
    // clang-format off
    std::array<float, 24> vertices{
      xpos,     ypos + h,   0.0f, 1.0f,
      xpos,     ypos,       0.0f, 0.0f,
      xpos + w, ypos,       1.0f, 0.0f,
      xpos,     ypos + h,   0.0f, 1.0f,
      xpos + w, ypos,       1.0f, 0.0f,
      xpos + w, ypos + h,   1.0f, 1.0f,
    };
    // clang-format on

    // Recover texture, update buffer and rener
    glBindTexture(GL_TEXTURE_2D, cd.texture_ids[char_idx]);

    // update content of VBO memory
    glBindBuffer(GL_ARRAY_BUFFER, bd.VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // render quad
    glDrawArrays(GL_TRIANGLES, 0, 6);

    /* Advance cursors for next glyph
     * The `advance` parameter is measured in 1/64 parts of a pixel
     * To get the vale in pixels, bitshift by 6. Remember that 2^6 = 64.
     */
    x += gsl::narrow_cast<float>(cd.advances[char_idx] >> 6) * dd.scale;
  }
}

void surge::atom::text::draw(GLuint shader_program, const buffer_data &bd, const charmap_data &cd,
                             const draw_data &dd, unsigned long long number) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::text::draw(number)");
#endif

  using std::snprintf;

  // Parse number into array of digits (repreented by chars)
  constexpr auto max_score_digits{std::numeric_limits<unsigned long long>::digits10};
  std::array<char, max_score_digits + 1> digits{};
  std::fill(digits.begin(), digits.end(), '\0');
  snprintf(digits.data(), max_score_digits, "%llu", number);

  draw(shader_program, bd, cd, dd, digits.data());
}