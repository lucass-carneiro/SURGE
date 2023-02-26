#ifndef SURGE_WINDOW_HPP
#define SURGE_WINDOW_HPP

#include "allocators/global_allocators.hpp"
#include "lua/lua_vm.hpp"
#include "options.hpp"

// clang-format off
#include "opengl/headers.hpp"

// #include <imgui.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <implot.h>
// clang-format on

#include <algorithm>
#include <cstddef>
#include <gsl/gsl-lite.hpp>
#include <memory>
#include <optional>
#include <tl/expected.hpp>
#include <tuple>

namespace surge {

void glfw_error_callback(int code, const char *description) noexcept;

void framebuffer_size_callback(GLFWwindow *, int width, int height) noexcept;

void glfw_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) noexcept;

void glfw_mouse_button_callback(GLFWwindow *window, int button, int action, int mods) noexcept;

void glfw_scroll_callback(GLFWwindow *window, double xoffset, double yoffset) noexcept;

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

  [[nodiscard]] inline auto get_config() const noexcept
      -> const std::optional<lua_engine_config> & {
    return engine_config;
  }

  [[nodiscard]] inline auto get_frame_dt() const noexcept -> double { return frame_dt; };

  inline auto poll_events() const noexcept { glfwPollEvents(); }

  inline void frame_timer_reset_and_start() const noexcept { glfwSetTime(double{0}); }

  inline void frame_timmer_compute_dt() noexcept { frame_dt = glfwGetTime(); }

  inline void clear_framebuffer() noexcept {
    glClearColor(engine_config->clear_color[0], engine_config->clear_color[1],
                 engine_config->clear_color[2], engine_config->clear_color[3]);
    glClear(GL_COLOR_BUFFER_BIT);
  }

  [[nodiscard]] inline auto get_key(int key) const noexcept -> int {
    return glfwGetKey(window.get(), key);
  };

  [[nodiscard]] inline auto get_shader_program() const noexcept -> GLuint {
    return sprite_shader.value_or(0);
  }

  [[nodiscard]] inline auto get_projection() const noexcept -> const glm::mat4 & {
    return projection_matrix;
  }

  [[nodiscard]] inline auto get_view() const noexcept -> const glm::mat4 & { return view_matrix; }

  [[nodiscard]] inline auto get_cursor_pos() const noexcept -> std::tuple<double, double> {
    double x{0}, y{0};
    glfwGetCursorPos(window.get(), &x, &y);
    return std::make_tuple(x, y);
  }

  ~global_engine_window();

  global_engine_window(const global_engine_window &) = delete;
  global_engine_window(global_engine_window &&) = delete;

  auto operator=(global_engine_window) -> global_engine_window & = delete;

  auto operator=(const global_engine_window &) -> global_engine_window & = delete;

  auto operator=(global_engine_window &&) -> global_engine_window & = delete;

private:
  global_engine_window() : window{nullptr, glfwDestroyWindow} {}

  std::optional<lua_engine_config> engine_config{};

  bool window_init_success{false};

  std::unique_ptr<GLFWwindow, void (*)(GLFWwindow *)> window;

  double frame_dt{0};

  std::optional<GLuint> sprite_shader{};
  glm::mat4 view_matrix{1.0f}, projection_matrix{1.0f};

  /**
   * Querry the existing available monitors.
   *
   * @return The monitor array if the monitors could or false.
   */
  auto querry_available_monitors() noexcept -> std::optional<std::pair<GLFWmonitor **, int>>;
};

} // namespace surge

#endif // SURGE_WINDOW_HPP