#include "font_cache.hpp"

#include "allocators.hpp"
#include "logging.hpp"

// clang-format off
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/yaml.h>
#include <EASTL/vector.h>
// clang-format on

#include <string>

auto surge::fonts::init(const char *config_file) noexcept -> bool {

  /******************
   * Config parsing *
   ******************/
  log_info("Parsing font data in engine config file %s", config_file);

  using font_name_vec_t = eastl::vector<std::string, allocators::eastl_allocators::gp_allocator>;
  font_name_vec_t font_names;

  try {
    const auto cf{YAML::LoadFile(config_file)};

    const YAML::Node &font_cache{cf["font_cache"]};
    font_names.reserve(font_cache.size());

    for (const auto &name : font_cache) {
      font_names.push_back(name.as<std::string>());
    }

  } catch (const std::exception &e) {
    log_error("Unable to load %s: %s", config_file, e.what());
  }

  for (const auto &name : font_names) {
    log_info("%s", name.c_str());
  }

  return true;
}