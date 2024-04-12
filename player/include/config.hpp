#ifndef SURGE_CONFIG_HPP
#define SURGE_CONFIG_HPP

#include "container_types.hpp"
#include "error_types.hpp"

#include <tl/expected.hpp>

namespace surge::config {

struct window_resolution {
  int width;
  int height;
};

struct clear_color {
  float r;
  float g;
  float b;
  float a;
};

struct window_attrs {
  string name;
  int monitor_index;
  bool windowed;
  bool cursor;
  bool vsync;
  bool MSAA;
};

struct config_data {
  window_resolution wr;
  clear_color ccl;
  window_attrs wattrs;
  string module;
};

auto parse_config() noexcept -> tl::expected<config_data, error>;

} // namespace surge::config

#endif // SURGE_CONFIG_HPP