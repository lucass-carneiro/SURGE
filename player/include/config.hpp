#ifndef SURGE_CONFIG_HPP
#define SURGE_CONFIG_HPP

#include <string>
#include <tl/expected.hpp>
#include <tuple>
#include <yaml-cpp/yaml.h>

namespace surge::config {

enum class cli_error { config_file_load, config_file_parse };

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
  std::string name;
  bool windowed;
  bool cursor;
  bool vsync;
};

using config_data = std::tuple<window_resolution, clear_color, window_attrs>;

auto parse_config() noexcept -> tl::expected<config_data, cli_error>;

} // namespace surge::config

#endif // SURGE_CONFIG_HPP