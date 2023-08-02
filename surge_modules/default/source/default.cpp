#include "default.hpp"

#include "allocators.hpp"
#include "logging.hpp"
#include "logo.hpp"

// clang-format off
#include <EASTL/bonus/ring_buffer.h>
#include <EASTL/vector.h>
// clang-format on

static bool show_debug_stats = false;

static eastl::ring_buffer<double,
                          eastl::vector<double, surge::allocators::eastl_allocators::gp_allocator>>
    frame_time_buffer;
static double dt_avg{0};

static constexpr const int sample_size = 100;

extern "C" {

SURGE_MODULE_EXPORT auto on_load(GLFWwindow *) noexcept -> bool {
  log_info("Loading default module");

  frame_time_buffer.reserve(sample_size);
  for (auto &dt : frame_time_buffer) {
    dt = 0.0;
  }

  return true;
}

SURGE_MODULE_EXPORT void on_unload() noexcept {
  log_info("Unloading default module");
  frame_time_buffer.clear();
}

SURGE_MODULE_EXPORT void draw() noexcept {
  // TODO
}

SURGE_MODULE_EXPORT void update(double dt) noexcept {
  frame_time_buffer.push_back(dt);
  double dt_sum{0};
  for (const auto dt : frame_time_buffer) {
    dt_sum += dt;
  }
  dt_avg = dt_sum / static_cast<double>(sample_size);
}

SURGE_MODULE_EXPORT void keyboard_event(GLFWwindow *, int key, int, int action, int) noexcept {
  if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
    show_debug_stats = !show_debug_stats;
  }
}

SURGE_MODULE_EXPORT void mouse_button_event(GLFWwindow *, int, int, int) noexcept {
  // TODO
}

SURGE_MODULE_EXPORT void mouse_scroll_event(GLFWwindow *, double, double) noexcept {
  // TODO
}
}