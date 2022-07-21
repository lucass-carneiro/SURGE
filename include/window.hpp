#ifndef SURGE_WINDOW_HPP
#define SURGE_WINDOW_HPP

#include "arena_allocator.hpp"
#include "options.hpp"

// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <EASTL/vector.h>
// clang-format on

#include <cstddef>
#include <memory>
#include <optional>
#include <utility>

namespace surge {

void glfw_error_callback(int code, const char *description) noexcept;

void framebuffer_size_callback(GLFWwindow *, int width, int height) noexcept;

/**
 * Querry the existing available monitors.
 *
 * @return The monitor array if the monitors could or false.
 */
auto querry_available_monitors() noexcept
    -> std::optional<std::pair<GLFWmonitor **, std::size_t>>;

class shader {
public:
  shader(const char *n, GLenum t, const char *src)
      : name{n}, source{src}, type{t} {}

  [[nodiscard]] inline auto is_valid() const noexcept -> bool {
    return handle.has_value();
  }

  [[nodiscard]] inline auto get_name() const noexcept -> const char * {
    return name;
  }

  [[nodiscard]] inline auto get_source() const noexcept -> const char * {
    return source;
  }

  [[nodiscard]] inline auto get_type() const noexcept -> GLenum { return type; }

  [[nodiscard]] inline auto get_handle() const noexcept
      -> std::optional<GLuint> {
    return handle;
  }

  void compile() noexcept;

private:
  const char *name;
  const char *source;
  const GLenum type;
  std::optional<GLuint> handle{};
};

auto link_shaders(shader &vertex_shader, shader &fragment_shader) noexcept
    -> std::optional<GLuint>;

} // namespace surge

#endif // SURGE_WINDOW_HPP