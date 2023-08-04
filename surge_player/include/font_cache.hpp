#ifndef SURGE_FONT_CACHE_HPP
#define SURGE_FONT_CACHE_HPP

#include "allocators.hpp"
#include "renderer.hpp"

// clang-format off
#include <ft2build.h>
#include FT_FREETYPE_H

#include <freetype/ftimage.h>
#include <freetype/fttypes.h>

#include <glm/ext/vector_float3.hpp>

#include <EASTL/vector.h>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
// clang-format on

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace surge::fonts {

struct font_system_context {
  using face_vec_t = eastl::vector<FT_Face, allocators::eastl_allocators::gp_allocator>;

  FT_Library library;
  face_vec_t faces;

  GLuint text_shader_program;

  glm::mat4 windo_proj;
  GLuint VAO;
  GLuint VBO;
};

struct charmap {
  using tid_vec_t = eastl::vector<GLuint, allocators::eastl_allocators::gp_allocator>;
  using size_vec_t = eastl::vector<FT_UInt, allocators::eastl_allocators::gp_allocator>;
  using bea_vec_t = eastl::vector<FT_Int, allocators::eastl_allocators::gp_allocator>;
  using adv_vec_t = eastl::vector<FT_Pos, allocators::eastl_allocators::gp_allocator>;

  std::size_t chars_per_face;

  tid_vec_t texture_ids;
  size_vec_t sizes_x;
  size_vec_t sizes_y;
  bea_vec_t bearings_x;
  bea_vec_t bearings_y;
  adv_vec_t advances;
};

auto init(GLFWwindow *window, const char *config_file) noexcept
    -> std::optional<font_system_context>;
void terminate(font_system_context &ctx) noexcept;

auto create_character_maps(font_system_context &ctx, FT_UInt pixel_height) noexcept
    -> std::optional<charmap>;

void render_text(font_system_context &ctx, charmap &map, std::uint64_t face_idx,
                 glm::vec3 pos_scale, glm::vec3 color, std::string_view text) noexcept;

} // namespace surge::fonts

#endif // SURGE_FONT_CACHE_HPP