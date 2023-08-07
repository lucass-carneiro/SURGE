#ifndef SURGE_RENDERER_HPP
#define SURGE_RENDERER_HPP

// clang-format off
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <glm/glm.hpp>
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

namespace uniforms {

void set(GLuint program_handle, const char *uniform_name, bool value) noexcept;
void set(GLuint program_handle, const char *uniform_name, GLint value) noexcept;
void set(GLuint program_handle, const char *uniform_name, float value) noexcept;
void set(GLuint program_handle, const char *uniform_name, const glm::bvec2 &value) noexcept;
void set(GLuint program_handle, const char *uniform_name, const glm::ivec2 &value) noexcept;
void set(GLuint program_handle, const char *uniform_name, const glm::fvec2 &value) noexcept;
void set(GLuint program_handle, const char *uniform_name, const glm::bvec3 &value) noexcept;
void set(GLuint program_handle, const char *uniform_name, const glm::ivec3 &value) noexcept;
void set(GLuint program_handle, const char *uniform_name, const glm::fvec3 &value) noexcept;
void set(GLuint program_handle, const char *uniform_name, const glm::bvec4 &value) noexcept;
void set(GLuint program_handle, const char *uniform_name, const glm::ivec4 &value) noexcept;
void set(GLuint program_handle, const char *uniform_name, const glm::fvec4 &value) noexcept;
void set(GLuint program_handle, const char *uniform_name, const glm::mat4 &value) noexcept;

} // namespace uniforms

namespace image {

struct context {
  // Uniform data
  glm::vec2 dimentions{0.0f};
  glm::vec2 ds{0.0f};

  GLuint shader_program;
  GLuint texture_id;
  GLuint VAO;
  GLuint VBO;
  GLuint EBO;
};

struct draw_context {
  glm::mat4 projection;
  glm::mat4 view;
  glm::vec3 pos;
  glm::vec3 scale;
  bool h_flip{false};
  bool v_flip{false};
};

auto create(const char *p) noexcept -> std::optional<context>;
void draw(context &ctx, draw_context &dctx) noexcept;

} // namespace image

} // namespace surge::renderer

#endif // SURGE_RENDERER_HPP
