#include "font_cache.hpp"

#include "allocators.hpp"
#include "files.hpp"
#include "logging.hpp"
#include "renderer.hpp"
#include "window.hpp"

// clang-format off
#include <cstddef>
#include <freetype/freetype.h>
#include <freetype/ftsystem.h>
#include <freetype/ftmodapi.h>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/vector_float3.hpp>
#include <string_view>
#include <yaml-cpp/yaml.h>
#include <yaml-cpp/node/node.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
// clang-format on

#include <array>
#include <cstdint>
#include <gsl/gsl-lite.hpp>
#include <string>

static auto FT_malloc(FT_Memory, long size) noexcept -> void * {
  return surge::allocators::mimalloc::malloc(size);
}

static void FT_free(FT_Memory, void *block) noexcept { surge::allocators::mimalloc::free(block); }

static auto FT_realloc(FT_Memory, long, long new_size, void *block) noexcept -> void * {
  return surge::allocators::mimalloc::realloc(block, new_size);
}

static FT_MemoryRec_ ft_mimalloc{nullptr, FT_malloc, FT_free, FT_realloc};

auto surge::fonts::init(GLFWwindow *window, const char *config_file) noexcept
    -> std::optional<font_system_context> {
  /******************
   * Config parsing *
   ******************/
  log_info("Parsing font data in engine config file %s", config_file);

  using font_name_vec_t = eastl::vector<std::string, allocators::eastl_allocators::gp_allocator>;
  font_name_vec_t font_names;

  try {
    const auto cf{YAML::LoadFile(config_file)};

    const YAML::Node &font_cache{cf["font_cache"]};
    font_names.reserve(font_cache.size());

    for (const auto &name : font_cache) {
      font_names.push_back(name.as<std::string>());
    }

  } catch (const std::exception &e) {
    log_error("Unable to load %s: %s", config_file, e.what());
    return {};
  }

  log_info("Adding the following fonts to the font cache:");
  for (const auto &name : font_names) {
    if (!files::validate_path(name.c_str())) {
      log_error("Invalid font %s", name.c_str());
      return {};
    };
    log_info("  %s", name.c_str());
  }

  /********************
   * Loading FreeType *
   ********************/
  log_info("Creating FreeType library");

  FT_Library lib{};
  auto status{FT_New_Library(&ft_mimalloc, &lib)};

  if (status != 0) {
    log_error("Error creating FreeType library: %s", FT_Error_String(status));
    return {};
  }

  log_info("Adding FreeType modules");
  FT_Add_Default_Modules(lib);

  log_info("Allocating face vector");
  font_system_context::face_vec_t face_vec;
  face_vec.reserve(font_names.size());

  log_info("Loading faces");
  for (const auto &name : font_names) {
    FT_Face face{};
    const auto status{FT_New_Face(lib, name.c_str(), 0, &face)};

    if (status != 0) {
      log_error("Error loading face %s: %s", name.c_str(), FT_Error_String(status));
      FT_Done_Library(lib);
      return {};
    } else
      face_vec.push_back(face);
  }

  /*************************
   * Compilig text shaders *
   *************************/
  log_info("Creating text shader program");
  const auto shader_handle{
      renderer::create_shader_program("shaders/text.vert", "shaders/text.frag")};
  if (!shader_handle) {
    log_error("Unable to create text shader program.");

    for (auto &face : face_vec) {
      FT_Done_Face(face);
    }
    FT_Done_Library(lib);

    return {};
  }

  /**************
   * Projection *
   **************/
  log_info("Constructing text projection matrix");
  const auto [ww, wh] = window::get_dims(window);
  const auto windo_proj{glm::ortho(0.0f, ww, 0.0f, wh)};

  /***********************************************
   * Getting orthographic projection from window *
   ***********************************************/
  log_info("Creating text vertex buffers");
  GLuint VAO{0}, VBO{0};
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);

  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  log_info("Font cache initialized");
  return font_system_context{lib, face_vec, *shader_handle, windo_proj, VAO, VBO, wh};
}

void surge::fonts::terminate(font_system_context &ctx) noexcept {
  log_info("Closing faces");
  for (const auto &face : ctx.faces) {
    FT_Done_Face(face);
  }

  log_info("Terminating FreeType library");
  const auto status{FT_Done_Library(ctx.library)};
  if (status != 0) {
    log_error("Unable to terminate FreeType library: %s", FT_Error_String(status));
  }
}

auto surge::fonts::create_character_maps(font_system_context &ctx, FT_UInt pixel_height) noexcept
    -> std::optional<charmap> {
  log_info("Creating character maps");

  /*We will only save the printable ASCII characters, ranging from codes
   * 32 to 126 (see https://en.cppreference.com/w/cpp/language/ascii)
   *
   * Each array in the charcater map will store (126 - 32) character data for each face loaded in
   * the context
   */
  constexpr const FT_ULong char_start{32};
  constexpr const FT_ULong char_end{126};
  constexpr const auto chars_per_face{char_end - char_start};
  const auto charmap_sizes{chars_per_face * ctx.faces.size()};

  log_info("Allocating memory for character maps");
  charmap map;
  map.chars_per_face = chars_per_face;
  map.texture_ids.reserve(charmap_sizes);
  map.sizes_x.reserve(charmap_sizes);
  map.sizes_y.reserve(charmap_sizes);
  map.bearings_x.reserve(charmap_sizes);
  map.bearings_y.reserve(charmap_sizes);
  map.advances.reserve(charmap_sizes);

  // Loop over faces
  for (auto &face : ctx.faces) {
    log_info("Face name: %s", face->family_name);

    log_info("  Setting size");
    auto status{FT_Set_Pixel_Sizes(face, 0, pixel_height)};
    if (status != 0) {
      log_error("Unable to set face %s size: %s", face->family_name, FT_Error_String(status));
      return {};
    }

    // Loop over characters
    log_info("  Loading characters");
    for (auto c = char_start; c <= char_end; c++) {

      status = FT_Load_Char(face, c, FT_LOAD_RENDER);
      if (status != 0) {
        log_error("Unable to load character %c for face %s: %s", static_cast<char>(c),
                  face->family_name, FT_Error_String(status));
        return {};
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
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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

    glBindTexture(GL_TEXTURE_2D, 0);
  }

  return map;
}

void surge::fonts::render_text(font_system_context &ctx, charmap &map, std::uint64_t face_idx,
                               glm::vec3 draw_pos_scale, glm::vec3 color,
                               std::string_view text) noexcept {

  // Set OpenGL state
  glUseProgram(ctx.text_shader_program);

  glUniformMatrix4fv(glGetUniformLocation(ctx.text_shader_program, "projection"), 1, GL_FALSE,
                     glm::value_ptr(ctx.windo_proj));
  glUniform3f(glGetUniformLocation(ctx.text_shader_program, "textColor"), color[0], color[1],
              color[2]);
  glActiveTexture(GL_TEXTURE0);

  glBindVertexArray(ctx.VAO);

  // This makes the text y position relative to the window's top left corner growing downwards,
  // which is more intuitive in graphycs
  const auto pos_scale{
      glm::vec3{draw_pos_scale[0], ctx.window_height - draw_pos_scale[1], draw_pos_scale[2]}};

  float x{pos_scale[0]};
  const float y{pos_scale[1]}, scale{pos_scale[2]};

  for (const auto c : text) {

    // Recover metrics
    const auto char_idx{static_cast<std::size_t>(c - 32) + face_idx * (map.chars_per_face + 1)};

    const float bearing_x{gsl::narrow_cast<float>(map.bearings_x[char_idx])},
        bearing_y{gsl::narrow_cast<float>(map.bearings_y[char_idx])};

    const float size_x{gsl::narrow_cast<float>(map.sizes_x[char_idx])},
        size_y{gsl::narrow_cast<float>(map.sizes_y[char_idx])};

    const float xpos{x + bearing_x * scale};
    const float ypos{y - (size_y - bearing_y) * scale};

    const float w{size_x * scale};
    const float h{size_y * scale};

    // Update VBO
    // clang-format off
    std::array<float, 24> vertices{{
      xpos,     ypos + h,   0.0f, 0.0f, // 1
      xpos,     ypos,       0.0f, 1.0f, // 2
      xpos + w, ypos,       1.0f, 1.0f, // 3
      xpos,     ypos + h,   0.0f, 0.0f, // 4
      xpos + w, ypos,       1.0f, 1.0f, // 5
      xpos + w, ypos + h,   1.0f, 0.0f, // 6

    }};
    // clang-format on

    // Recover texture, update buffer and rener
    glBindTexture(GL_TEXTURE_2D, map.texture_ids[char_idx]);

    // update content of VBO memory
    glBindBuffer(GL_ARRAY_BUFFER, ctx.VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // render quad
    glDrawArrays(GL_TRIANGLES, 0, 6);

    /* Now advance cursors for next glyph (note that advance is number of 1/64 pixels) bitshift by 6
     * to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of
     * pixels))
     */
    x += gsl::narrow_cast<float>(map.advances[char_idx] >> 6) * scale;
  }
}
