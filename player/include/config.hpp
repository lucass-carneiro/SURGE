#ifndef SURGE_CONFIG_HPP
#define SURGE_CONFIG_HPP
#include "allocators.hpp"

#include <foonathan/memory/std_allocator.hpp>
#include <string>
#include <tl/expected.hpp>
#include <tuple>

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
using string = std::basic_string<
    char, std::char_traits<char>,
    foonathan::memory::std_allocator<char, allocators::mimalloc::fnm_allocator>>;

struct window_attrs {
  string name;
  int monitor_index;
  bool windowed;
  bool cursor;
  bool vsync;
};

struct config_data {
  window_resolution wr;
  clear_color ccl;
  window_attrs wattrs;
  string module;
};

auto parse_config() noexcept -> tl::expected<config_data, cli_error>;

} // namespace surge::config

#endif // SURGE_CONFIG_HPP