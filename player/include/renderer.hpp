#ifndef SURGE_RENDERER_HPP
#define SURGE_RENDERER_HPP

#include "allocators.hpp"
#include "config.hpp"

// clang-format off
#include <cstddef>
#include <glm/fwd.hpp>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <glm/glm.hpp>
// clang-format on

#include <EASTL/vector.h>
#include <tl/expected.hpp>

namespace surge::renderer {

enum class renderer_error { unrecognized_shader, shader_load_error, shader_link_error };

enum class capability : GLenum { depth_test = GL_DEPTH_TEST, blend = GL_BLEND };

enum class blend_src : GLenum { alpha = GL_SRC_ALPHA };

enum class blend_dest : GLenum { one_minus_src_alpha = GL_ONE_MINUS_SRC_ALPHA };

void enable(const capability cap) noexcept;
void disable(const capability cap) noexcept;

void blend_function(const blend_src src, const blend_dest dest) noexcept;

void clear(const config::clear_color &ccl) noexcept;

auto create_shader_program(const char *vertex_shader_path,
                           const char *fragment_shader_path) noexcept
    -> tl::expected<GLuint, renderer_error>;

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

enum class image_error : std::uint32_t { load_error = 1, stbi_error = 2, shader_creation = 3 };

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
  glm::vec2 region_origin;
  glm::vec2 region_dims;
  bool h_flip{false};
  bool v_flip{false};
};

auto create(const char *p) noexcept -> tl::expected<context, image_error>;

void draw(const context &ctx, const draw_context &dctx) noexcept;
void draw(const context &ctx, draw_context &&dctx) noexcept;

} // namespace image

// Stands for static mesh object. It is an object with static geometry and fixed color that can be
// drawn. You pass the vertex data and how it shoul be interpreted (lines, pts, triangles, etc) and
// it is drawn on the screen with fixed color.
namespace smo {

enum class error : std::uint32_t { shader_creation = 1 };

enum class type : GLenum {
  points = GL_POINTS,
  line_strip = GL_LINE_STRIP,
  line_loop = GL_LINE_LOOP,
  lines = GL_LINES,
  triangles = GL_TRIANGLES
};

struct context {
  GLuint shader_program;
  GLuint VAO;
  GLuint VBO;
  GLsizei num_elements;
};

// Draws indexed triangles. Allows complex 3D meshes to be draw
struct indexed_context {
  GLuint shader_program;
  GLuint VAO;
  GLuint VBO;
  GLuint EBO;
  GLsizei num_elements;
};

// Draws non indexed elements (poitns, lines triangles)
struct draw_context {
  glm::mat4 projection;
  glm::mat4 view;
  glm::mat4 model;
  glm::vec4 color;
};

struct vertex_data {
  const float *data;
  std::size_t size;
};

struct index_vertex_data {
  const float *vertex_data;
  std::size_t vertex_data_size;

  const GLuint *index_data;
  std::size_t index_data_size;
};

auto create(GLuint shader_program, const index_vertex_data &vd) noexcept -> indexed_context;
auto create(GLuint shader_program, const vertex_data &vd) noexcept -> context;

auto create(const index_vertex_data &vd) noexcept -> tl::expected<indexed_context, error>;
auto create(const vertex_data &vd) noexcept -> tl::expected<context, error>;

void draw(const indexed_context &ctx, const draw_context &dctx);
void draw(type primitive_type, const context &vd, const draw_context &dc);

} // namespace smo

} // namespace surge::renderer

#endif // SURGE_RENDERER_HPP