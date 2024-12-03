#ifndef SURGE_CORE_CONFIG_HPP
#define SURGE_CORE_CONFIG_HPP

#include "sc_container_types.hpp"
#include "sc_error_types.hpp"

#include <tl/expected.hpp>

namespace surge::config {

struct window_resolution {
  int width{800};
  int height{600};
};

struct clear_color {
  float r{0.0f};
  float g{0.0f};
  float b{0.0f};
  float a{1.0f};
};

struct window_attrs {
  string name{"SURGE Window"};
  int monitor_index{0};
  bool windowed{true};
  bool cursor{true};
};

enum class renderer_backend { opengl, vulkan, none };

struct renderer_attrs {
  renderer_backend backend{renderer_backend::none};
  bool vsync{true};
  bool MSAA{true};
  bool fps_cap{true};
  int fps_cap_value{60};
};

struct config_data {
  window_resolution wr{};
  clear_color ccl{};
  window_attrs wattrs{};
  renderer_attrs rattrs{};
  string module{};
};

auto parse_config(renderer_backend backend) -> tl::expected<config_data, error>;

} // namespace surge::config

#endif // SURGE_CORE_CONFIG_HPP