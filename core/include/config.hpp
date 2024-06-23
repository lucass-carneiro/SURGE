#ifndef SURGE_CORE_CONFIG_HPP
#define SURGE_CORE_CONFIG_HPP

#include "container_types.hpp"
#include "error_types.hpp"

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
  bool vsync{true};
  bool MSAA{true};
  bool fps_cap{true};
  int fps_cap_value {60};
};

struct config_data {
  window_resolution wr{};
  clear_color ccl{};
  window_attrs wattrs{};
  string module{};
};

auto parse_config() noexcept -> tl::expected<config_data, error>;

} // namespace surge::config

#endif // SURGE_CORE_CONFIG_HPP