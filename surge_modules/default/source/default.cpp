#include "default.hpp"

#include "allocators.hpp"
#include "logging.hpp"
#include "logo.hpp"

#include <bgfx/bgfx.h>

// clang-format off
#include <EASTL/bonus/ring_buffer.h>
#include <EASTL/vector.h>
// clang-format on

extern "C" {

static bool show_debug_stats = false;

static eastl::ring_buffer<double,
                          eastl::vector<double, surge::allocators::eastl_allocators::gp_allocator>>
    frame_time_buffer;
static double dt_avg{0};

SURGE_MODULE_EXPORT void on_load() noexcept {
  log_info("Loading default module");

  frame_time_buffer.reserve(100);
  for (auto &dt : frame_time_buffer) {
    dt = 0.0;
  }
}

SURGE_MODULE_EXPORT void on_unload() noexcept {
  log_info("Unloading default module");
  frame_time_buffer.clear();
}

SURGE_MODULE_EXPORT void draw() noexcept {
  bgfx::dbgTextClear();

  if (show_debug_stats) {
    bgfx::setDebug(BGFX_DEBUG_STATS);
  } else {
    bgfx::setDebug(BGFX_DEBUG_TEXT);

    bgfx::dbgTextPrintf(1, 1, 0x0f, "Last dt, FPS = %.4f %.4f", dt_avg, 1.0 / dt_avg);

    std::uint16_t y{3};
    for (const auto &line : surge::cli::LOGO_LINES) {
      bgfx::dbgTextPrintf(1, y, 0x04, "%s", line);
      y++;
    }
    y++;

    bgfx::dbgTextPrintf(1, y, 0x0f, "\x1b[4;0mS\x1b[0muper");
    y++;
    bgfx::dbgTextPrintf(1, y, 0x0f, "\x1b[4;0mU\x1b[0mnder\x1b[4;0mR\x1b[0mated");
    y++;
    bgfx::dbgTextPrintf(1, y, 0x0f, "\x1b[4;0mG\x1b[0mame");
    y++;
    bgfx::dbgTextPrintf(1, y, 0x0f, "\x1b[4;0mE\x1b[0mngine");
    y++;
    y++;

    bgfx::dbgTextPrintf(1, y, 0x0f, "Press F1 to see bgfx stats");
  }
}

SURGE_MODULE_EXPORT void update(double dt) noexcept {
  frame_time_buffer.push_back(dt);
  double dt_sum{0};
  for (const auto dt : frame_time_buffer) {
    dt_sum += dt;
  }
  dt_avg = dt_sum / 100.0;
}

SURGE_MODULE_EXPORT void keyboard_event(GLFWwindow *, int key, int, int action, int) noexcept {
  if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
    show_debug_stats = !show_debug_stats;
  }
}

SURGE_MODULE_EXPORT void mouse_button_event(GLFWwindow *window, int, int, int) noexcept {
  double x{0}, y{0};
  glfwGetCursorPos(window, &x, &y);
  log_info("Click at (%f, %f)", x, y);
}

SURGE_MODULE_EXPORT void mouse_scroll_event(GLFWwindow *, double, double) noexcept {
  // todo
}
}