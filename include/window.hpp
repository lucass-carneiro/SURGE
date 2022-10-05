#ifndef SURGE_WINDOW_HPP
#define SURGE_WINDOW_HPP

#include "allocators/global_allocators.hpp"
#include "lua_vm.hpp"
#include "options.hpp"

// clang-format off
#include <fmt/format.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on

#include <cstddef>
#include <gsl/gsl-lite.hpp>
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

  [[nodiscard]] inline auto should_close() noexcept -> bool {
    return static_cast<bool>(glfwWindowShouldClose(window.get()));
  }

  inline void swap_buffers() noexcept { glfwSwapBuffers(window.get()); }

  [[nodiscard]] inline auto get_clear_color_r() const noexcept -> float {
    return static_cast<float>(clear_color_r);
  }

  [[nodiscard]] inline auto get_clear_color_g() const noexcept -> float {
    return static_cast<float>(clear_color_g);
  }

  [[nodiscard]] inline auto get_clear_color_b() const noexcept -> float {
    return static_cast<float>(clear_color_b);
  }

  [[nodiscard]] inline auto get_clear_color_a() const noexcept -> float {
    return static_cast<float>(clear_color_a);
  }

  [[nodiscard]] inline auto get_frame_dt() const noexcept -> double { return previous_frame_dt; }

  inline void frame_timer_reset_and_start() const noexcept { glfwSetTime(double{0}); }

  inline void frame_timmer_compute_dt() noexcept { previous_frame_dt = glfwGetTime(); }

  ~global_engine_window();

  global_engine_window(const global_engine_window &) = delete;
  global_engine_window(global_engine_window &&) = delete;

  auto operator=(global_engine_window) -> global_engine_window & = delete;

  auto operator=(const global_engine_window &) -> global_engine_window & = delete;

  auto operator=(global_engine_window &&) -> global_engine_window & = delete;

private:
  global_engine_window() : window{nullptr, glfwDestroyWindow} {}

  lua_Integer window_width{800};
  lua_Integer window_height{600};
  lua_String window_name{"Default window name"};
  lua_Boolean windowed{true};
  lua_Integer window_monitor_index{0};
  lua_Number clear_color_r = lua_Number{0};
  lua_Number clear_color_g = lua_Number{0};
  lua_Number clear_color_b = lua_Number{0};
  lua_Number clear_color_a = lua_Number{1};

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