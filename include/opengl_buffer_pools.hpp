#ifndef SURGE_OPENGL_BUFFER_POOLS
#define SURGE_OPENGL_BUFFER_POOLS

#include "window.hpp"

#include <array>

namespace surge {

template <GLsizei n> class static_vao_buffer_pool {
public:
  inline static_vao_buffer_pool() noexcept {
    static_assert(n > 0, "Cannot initialize a negative number of VAOs");
    glGenVertexArrays(n, VAOs.data());
    should_destroy = true; // NOLINT
  }

  inline ~static_vao_buffer_pool() { destroy(); }

  static_vao_buffer_pool(const static_vao_buffer_pool &) = delete;
  static_vao_buffer_pool(static_vao_buffer_pool &&) = delete;

  auto operator=(static_vao_buffer_pool) -> static_vao_buffer_pool & = delete;

  auto operator=(const static_vao_buffer_pool &)
      -> static_vao_buffer_pool & = delete;

  auto operator=(static_vao_buffer_pool &&)
      -> static_vao_buffer_pool & = delete;

  inline void destroy() noexcept {
    if (should_destroy) {
      glDeleteVertexArrays(n, VAOs.data());
      should_destroy = false;
    }
  }

  template <GLsizei idx = 0> inline void bind() noexcept {
    static_assert(idx >= 0 && idx < n,
                  "Unable to bind VAO because it's index is invalid");
    glBindVertexArray(VAOs[idx]);
  }

  inline void unbind() noexcept { glBindVertexArray(0); }

  inline void enable(GLuint index) noexcept {
    glEnableVertexAttribArray(index);
  }

private:
  std::array<GLuint, n> VAOs;
  bool should_destroy{false};
};

template <GLsizei n> class static_buffer_pool {
public:
  inline static_buffer_pool() noexcept {
    static_assert(n > 0, "Cannot initialize a negative number of buffers");
    glGenBuffers(n, buffers.data());
    should_destroy = true; // NOLINT
  }

  inline ~static_buffer_pool() { destroy(); }

  static_buffer_pool(const static_buffer_pool &) = delete;
  static_buffer_pool(static_buffer_pool &&) = delete;

  auto operator=(static_buffer_pool) -> static_buffer_pool & = delete;

  auto operator=(const static_buffer_pool &) -> static_buffer_pool & = delete;

  auto operator=(static_buffer_pool &&) -> static_buffer_pool & = delete;

  inline void destroy() noexcept {
    if (should_destroy) {
      glDeleteBuffers(n, buffers.data());
      should_destroy = false;
    }
  }

  template <GLsizei idx = 0> inline void bind(GLenum target) noexcept {
    static_assert(idx >= 0 && idx < n,
                  "Unable to bind buffer because it's index is invalid");
    glBindBuffer(target, buffers[idx]);
  }

  inline void unbind(GLenum target) noexcept { glBindBuffer(target, 0); }

  inline void transfer_data(GLenum target, GLsizeiptr size, const void *data,
                            GLenum usage) noexcept {
    glBufferData(target, size, data, usage);
  }

private:
  std::array<GLuint, n> buffers;
  bool should_destroy{false};
};

} // namespace surge

#endif // SURGE_OPENGL_BUFFER_POOLS