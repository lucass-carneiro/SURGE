#ifndef SURGE_RENDERER_HPP
#define SURGE_RENDERER_HPP

// clang-format off
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
// clang-format on

#include <optional>

namespace surge::renderer {

struct clear_color {
  float r{0};
  float g{0};
  float b{0};
  float a{1};
};

enum class capability : GLenum { depth_test = GL_DEPTH_TEST, blend = GL_BLEND };

enum class blend_src : GLenum { alpha = GL_SRC_ALPHA };

enum class blend_dest : GLenum { one_minus_src_alpha = GL_ONE_MINUS_SRC_ALPHA };

void enable(const capability cap) noexcept;
void disable(const capability cap) noexcept;

void blend_function(const blend_src src, const blend_dest dest) noexcept;

void clear(const clear_color &ccl) noexcept;

auto create_shader_program(const char *vertex_shader_path,
                           const char *fragment_shader_path) noexcept -> std::optional<GLuint>;

} // namespace surge::renderer

#endif // SURGE_RENDERER_HPP
