#include "default.hpp"

#include "allocators.hpp"
#include "font_cache.hpp"
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

std::optional<surge::fonts::font_system_context> freetype_ctx{};
std::optional<surge::fonts::charmap> char_map{};

extern "C" {

SURGE_MODULE_EXPORT auto on_load(GLFWwindow *window) noexcept -> bool {
  using namespace surge;

  log_info("Loading default module");

  /*******************
   * Init font cache *
   *******************/
  log_info("Initializing font cache");
  freetype_ctx = fonts::init(window, "config.yaml");
  if (!freetype_ctx) {
    return false;
  }

  char_map = fonts::create_character_maps(*freetype_ctx, 100);
  if (!char_map) {
    return false;
  }

  frame_time_buffer.reserve(sample_size);
  for (auto &dt : frame_time_buffer) {
    dt = 0.0;
  }

  return true;
}

SURGE_MODULE_EXPORT void on_unload() noexcept {
  using namespace surge;

  log_info("Unloading default module");
  frame_time_buffer.clear();

  log_info("Terminating font cache");
  fonts::terminate(*freetype_ctx);
}

SURGE_MODULE_EXPORT void draw() noexcept {
  using namespace surge;

  fonts::render_text(*freetype_ctx, *char_map, 0, glm::vec3{10.0f, 80.0f, 1.0f},
                     glm::vec3{220.0f / 256.0f, 20.0f / 256.0f, 60.0f / 256.0f}, "SURGE");
  fonts::render_text(*freetype_ctx, *char_map, 1, glm::vec3{10.0f, 130.0f, 0.3f},
                     glm::vec3{0.0f, 0.0f, 0.0f}, "The Super Underrated Game Engine");
  fonts::render_text(*freetype_ctx, *char_map, 1, glm::vec3{10.0f, 160.0f, 0.3f},
                     glm::vec3{0.0f, 0.0f, 0.0f}, "Created with love by the Ninja Sheep");
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