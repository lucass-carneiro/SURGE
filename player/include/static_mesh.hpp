#ifndef SURGE_ATOM_STATIC_MESH_HPP
#define SURGE_ATOM_STATIC_MESH_HPP

#include "allocators.hpp"
#include "files.hpp"
#include "renderer.hpp"

#include <array>
#include <cstddef>
#include <foonathan/memory/std_allocator.hpp>
#include <tuple>
#include <vector>

/**
 * Drawable mesh with the following properties:
 *
 * Dynamic geometry: No
 * Textured: No
 * RGB colored: Yes
 */
namespace surge::atom::static_mesh {

template <typename T> using vec
    = std::vector<T, foonathan::memory::std_allocator<T, allocators::mimalloc::fnm_allocator>>;
template <typename T, std::size_t N> using arr = std::array<T, N>;

struct one_buffer_data {
  GLuint VAO;
  GLsizei elements;
};

template <std::size_t N> struct st_buffer_data {
  arr<GLuint, N> VAOs;
  arr<GLsizei, N> elements;
};

struct buffer_data {
  vec<GLuint> VAOs;
  vec<GLsizei> elements;
};

struct one_draw_data {
  glm::mat4 projection;
  glm::mat4 view;
  glm::mat4 model;
  glm::vec4 color;
};

template <std::size_t N> struct st_draw_data {
  arr<glm::mat4, N> projection;
  arr<glm::mat4, N> view;
  arr<glm::mat4, N> model;
  arr<glm::vec4, N> color;
};

struct draw_data {
  vec<glm::mat4> projection;
  vec<glm::mat4> view;
  vec<glm::mat4> model;
  vec<glm::vec4> color;
};

auto gen_triangle() noexcept -> std::tuple<GLuint, std::size_t>;
auto gen_square() noexcept -> std::tuple<GLuint, std::size_t>;

auto load(const char *path) noexcept -> tl::expected<one_buffer_data, files::file_error>;

void draw(GLuint shader_program, const one_buffer_data &bd, const one_draw_data &dd) noexcept;

} // namespace surge::atom::static_mesh

#endif // SURGE_ATOM_STATIC_MESH_HPP