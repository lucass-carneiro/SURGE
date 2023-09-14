#include "config.hpp"

#include "files.hpp"
#include "logging.hpp"

auto surge::config::parse_config() noexcept -> tl::expected<config_data, cli_error> {
  const auto config_file{files::load_file("config.yaml", false)};

  if (!config_file) {
    log_error("Unable to load config.yaml file");
    return tl::unexpected(cli_error::config_file_load);
  }

  try {
    const auto yaml_file{YAML::Load(reinterpret_cast<const char *>(config_file->data()))};

    // Window resolution
    const auto width{yaml_file["window"]["resolution"]["width"].as<int>()};
    const auto height{yaml_file["window"]["resolution"]["height"].as<int>()};
    const window_resolution res{width, height};

    // Window clear color
    const clear_color ccl{0.0f, 0.0f, 0.0f, 0.0f};

    // Window attributes
    const window_attrs attrs{"SURGE Window", true, true, true};

    return std::make_tuple(res, ccl, attrs);

  } catch (const std::exception &e) {
    log_error("Error while parsing config.yaml: %s", e.what());
    return tl::unexpected(cli_error::config_file_parse);
  }
}