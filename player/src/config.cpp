#include "config.hpp"

#include "files.hpp"
#include "logging.hpp"

auto surge::config::parse_config() noexcept -> tl::expected<config_data, cli_error> {
  const auto config_file{files::load_file("config.yaml", true)};

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
    const auto r{yaml_file["renderer"]["clear_color"]["r"].as<float>()};
    const auto g{yaml_file["renderer"]["clear_color"]["g"].as<float>()};
    const auto b{yaml_file["renderer"]["clear_color"]["b"].as<float>()};
    const auto a{yaml_file["renderer"]["clear_color"]["a"].as<float>()};
    const clear_color ccl{r, g, b, a};

    // Window attributes
    const auto name{yaml_file["window"]["name"].as<std::string>()};
    const auto monitor_index{yaml_file["window"]["monitor_index"].as<int>()};
    const auto windowed{yaml_file["window"]["windowed"].as<bool>()};
    const auto cursor{yaml_file["window"]["cursor"].as<bool>()};
    const auto vsync{yaml_file["window"]["VSync"]["enabled"].as<bool>()};
    const window_attrs attrs{"SURGE Window", monitor_index, windowed, cursor, vsync};

    // Module list
    module_list mlist{};

    for (const auto &node : yaml_file["modules"]) {
      auto module_name{node.as<std::string>()};
#ifdef SURGE_SYSTEM_Windows
      module_name.append(".dll");
#else
      module_name.insert(0, "lib");
      module_name
          .append(".so")
#endif
      mlist.push_back(module_name);
    }

    return std::make_tuple(res, ccl, attrs, mlist);

  } catch (const std::exception &e) {
    log_error("Error while parsing config.yaml: %s", e.what());
    return tl::unexpected(cli_error::config_file_parse);
  }
}