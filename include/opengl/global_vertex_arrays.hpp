#ifndef SURGE_GLOBAL_OPENGL_VERTEX_ARRAYS_HPP
#define SURGE_GLOBAL_OPENGL_VERTEX_ARRAYS_HPP

#include "headers.hpp"
#include "options.hpp"

#include <array>

namespace surge {

class global_opengl_vertex_arrays {
public:
  static auto get() noexcept -> global_opengl_vertex_arrays & {
    static global_opengl_vertex_arrays gba;
    return gba;
  }

  global_opengl_vertex_arrays(const global_opengl_vertex_arrays &) = delete;
  global_opengl_vertex_arrays(global_opengl_vertex_arrays &&) = delete;

  auto operator=(global_opengl_vertex_arrays) -> global_opengl_vertex_arrays & = delete;

  auto operator=(const global_opengl_vertex_arrays &) -> global_opengl_vertex_arrays & = delete;

  auto operator=(global_opengl_vertex_arrays &&) -> global_opengl_vertex_arrays & = delete;

  ~global_opengl_vertex_arrays() {
    glDeleteVertexArrays(SURGE_OPENGL_BUFFER_COUNT, vertex_arrays.data());
  }

  [[nodiscard]] auto data() const noexcept
      -> const std::array<GLuint, SURGE_OPENGL_BUFFER_COUNT> & {
    return vertex_arrays;
  }

private:
  global_opengl_vertex_arrays() {
    glGenVertexArrays(SURGE_OPENGL_BUFFER_COUNT, vertex_arrays.data());
  }

  std::array<GLuint, SURGE_OPENGL_BUFFER_COUNT> vertex_arrays{};
};

} // namespace surge

#endif // SURGE_GLOBAL_OPENGL_VERTEX_ARRAYS_HPP