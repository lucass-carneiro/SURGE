#ifndef SURGE_GLOBAL_OPENGL_BUFFERS_HPP
#define SURGE_GLOBAL_OPENGL_BUFFERS_HPP

#include "headers.hpp"
#include "options.hpp"

#include <array>

namespace surge {

class global_opengl_buffers {
public:
  static auto get() noexcept -> global_opengl_buffers & {
    static global_opengl_buffers gba;
    return gba;
  }

  global_opengl_buffers(const global_opengl_buffers &) = delete;
  global_opengl_buffers(global_opengl_buffers &&) = delete;

  auto operator=(global_opengl_buffers) -> global_opengl_buffers & = delete;

  auto operator=(const global_opengl_buffers &) -> global_opengl_buffers & = delete;

  auto operator=(global_opengl_buffers &&) -> global_opengl_buffers & = delete;

  ~global_opengl_buffers() { glDeleteBuffers(SURGE_OPENGL_BUFFER_COUNT, buffers.data()); }

  [[nodiscard]] auto data() const noexcept
      -> const std::array<GLuint, SURGE_OPENGL_BUFFER_COUNT> & {
    return buffers;
  }

private:
  global_opengl_buffers() { glGenBuffers(SURGE_OPENGL_BUFFER_COUNT, buffers.data()); }

  std::array<GLuint, SURGE_OPENGL_BUFFER_COUNT> buffers{};
};

} // namespace surge

#endif // SURGE_GLOBAL_OPENGL_BUFFERS_HPP