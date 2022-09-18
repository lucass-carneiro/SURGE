#ifndef SURGE_WINDOW_HPP
#define SURGE_WINDOW_HPP

#include "allocators/global_allocators.hpp"
#include "options.hpp"
#include "squirrel_bindings.hpp"

// clang-format off
#include <fmt/format.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on

#include <cstddef>
#include <memory>
#include <optional>
#include <tl/expected.hpp>
#include <utility>

namespace surge {

void glfw_error_callback(int code, const char *description) noexcept;

void framebuffer_size_callback(GLFWwindow *, int width, int height) noexcept;

auto glfw_allocate(std::size_t size, void *user) noexcept -> void *;

auto glfw_reallocate(void *block, std::size_t size, void *user) noexcept -> void *;

auto glfw_free(void *block, void *user) noexcept;

class global_engine_window {
public:
  static auto get() noexcept -> global_engine_window & {
    static global_engine_window gew;
    return gew;
  }

  auto init() noexcept -> bool;

  [[nodiscard]] inline auto should_close() noexcept -> bool {
    return static_cast<bool>(glfwWindowShouldClose(window.get()));
  }

  inline void swap_buffers() noexcept { glfwSwapBuffers(window.get()); }

  [[nodiscard]] inline auto get_clear_color_r() const noexcept -> SQFloat { return clear_color_r; }

  [[nodiscard]] inline auto get_clear_color_g() const noexcept -> SQFloat { return clear_color_g; }

  [[nodiscard]] inline auto get_clear_color_b() const noexcept -> SQFloat { return clear_color_b; }

  [[nodiscard]] inline auto get_clear_color_a() const noexcept -> SQFloat { return clear_color_a; }

  [[nodiscard]] inline auto get_frame_dt() const noexcept -> double { return previous_frame_dt; }

  inline void frame_timer_reset_and_start() const noexcept { glfwSetTime(double{0}); }

  inline void frame_timmer_compute_dt() noexcept { previous_frame_dt = glfwGetTime(); }

  ~global_engine_window();

  global_engine_window(const global_engine_window &) = delete;
  global_engine_window(global_engine_window &&) = delete;

  auto operator=(global_engine_window) -> global_engine_window & = delete;

  auto operator=(const global_engine_window &) -> global_engine_window & = delete;

  auto operator=(global_engine_window &&) -> global_engine_window & = delete;

  static const std::size_t subsystem_allocator_capacity;

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

  double previous_frame_dt{0};

  /**
   * Querry the existing available monitors.
   *
   * @return The monitor array if the monitors could or false.
   */
  auto querry_available_monitors() noexcept -> std::optional<std::pair<GLFWmonitor **, int>>;
};

} // namespace surge

#endif // SURGE_WINDOW_HPP