#ifndef SURGE_WINDOW_HPP
#define SURGE_WINDOW_HPP

#include "arena_allocator.hpp"
#include "options.hpp"
#include "squirrel_bindings.hpp"

// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <EASTL/vector.h>
// clang-format on

#include <cstddef>
#include <memory>
#include <optional>
#include <tl/expected.hpp>
#include <utility>

namespace surge {

void glfw_error_callback(int code, const char *description) noexcept;

void framebuffer_size_callback(GLFWwindow *, int width, int height) noexcept;

class global_engine_window {
public:
  static auto get() noexcept -> global_engine_window & {
    static global_engine_window gew;
    return gew;
  }

  auto init() noexcept -> bool;

  [[nodiscard]] auto should_close() noexcept -> bool;

  void swap_buffers() noexcept;

  [[nodiscard]] auto get_clear_color_r() const noexcept -> SQFloat {
    return clear_color_r;
  }

  [[nodiscard]] auto get_clear_color_g() const noexcept -> SQFloat {
    return clear_color_g;
  }

  [[nodiscard]] auto get_clear_color_b() const noexcept -> SQFloat {
    return clear_color_b;
  }

  [[nodiscard]] auto get_clear_color_a() const noexcept -> SQFloat {
    return clear_color_a;
  }

  ~global_engine_window();

  global_engine_window(const global_engine_window &) = delete;
  global_engine_window(global_engine_window &&) = delete;

  auto operator=(global_engine_window) -> global_engine_window & = delete;

  auto operator=(const global_engine_window &)
      -> global_engine_window & = delete;

  auto operator=(global_engine_window &&) -> global_engine_window & = delete;

private:
  global_engine_window() : window{nullptr, glfwDestroyWindow} {}

  int window_width = 800;
  int window_height = 600;
  const SQChar *window_name = "Default window name";
  SQBool windowed = SQBool{true};
  SQInteger window_monitor_index = SQInteger{0};
  SQFloat clear_color_r = SQFloat{0};
  SQFloat clear_color_g = SQFloat{0};
  SQFloat clear_color_b = SQFloat{0};
  SQFloat clear_color_a = SQFloat{1};

  bool glfw_init_success = false;
  std::unique_ptr<GLFWwindow, void (*)(GLFWwindow *)> window;

  /**
   * Querry the existing available monitors.
   *
   * @return The monitor array if the monitors could or false.
   */
  auto querry_available_monitors() noexcept
      -> std::optional<std::pair<GLFWmonitor **, int>>;
};

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