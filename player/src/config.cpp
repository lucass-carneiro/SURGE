#include "config.hpp"

#include "files.hpp"
#include "logging.hpp"

#include <cstring>
#include <filesystem>
#include <format>
#include <stdexcept>

// clang-format off
#include <ini.h>
// clang-format on

static auto config_handler(void *user, const char *section, const char *name,
                           const char *value) noexcept -> int {
  auto match{[&](const char *s, const char *n) {
    return strcmp(section, s) == 0 && strcmp(name, n) == 0;
  }};

  auto config{static_cast<surge::config::config_data *>(user)};

  if (match("window", "name")) {
    config->wattrs.name = surge::string{value};
  } else if (match("window", "monitor_index")) {
    config->wattrs.monitor_index = std::atoi(value);
  } else if (match("window", "windowed")) {
    config->wattrs.windowed = static_cast<bool>(std::atoi(value));
  } else if (match("window", "cursor")) {
    config->wattrs.cursor = static_cast<bool>(std::atoi(value));
  } else if (match("resolution", "width")) {
    config->wr.width = std::atoi(value);
  } else if (match("resolution", "height")) {
    config->wr.height = std::atoi(value);
  } else if (match("renderer", "VSync")) {
    config->wattrs.vsync = static_cast<bool>(std::atoi(value));
  } else if (match("renderer", "MSAA")) {
    config->wattrs.MSAA = static_cast<bool>(std::atoi(value));
  } else if (match("renderer", "cap_FPS")) {
    config->wattrs.fps_cap.first = static_cast<bool>(std::atoi(value));
  } else if (match("renderer", "FPS_cap_value")) {
    config->wattrs.fps_cap.second = static_cast<surge::u8>(std::atoi(value));
  } else if (match("clear_color", "r")) {
    config->ccl.r = std::strtof(value, nullptr);
  } else if (match("clear_color", "g")) {
    config->ccl.g = std::strtof(value, nullptr);
  } else if (match("clear_color", "b")) {
    config->ccl.b = std::strtof(value, nullptr);
  } else if (match("clear_color", "a")) {
    config->ccl.a = std::strtof(value, nullptr);
  } else if (match("modules", "first_module")) {
    config->module = surge::string{value};
#ifdef SURGE_SYSTEM_Windows
    config->module.append(".dll");
#else
    config->module.insert(0, "/");
    config->module.insert(0, std::filesystem::current_path().string());
    config->module.append(".so");
#endif
  } else {
    log_error("Unable section / name entry in config.ini: %s/%s", section, name);
    return 0;
  }

  return 1;
}

auto surge::config::parse_config() noexcept -> tl::expected<config_data, error> {
  auto config_file{files::load_file("config.ini", true)};

  if (!config_file) {
    log_error("Unable to load config.ini file");
    return tl::unexpected(error::config_file_load);
  }

  config_data cd{};
  const auto file_str{reinterpret_cast<const char *>(config_file->data())};

  if (ini_parse_string(file_str, config_handler, static_cast<void *>(&cd)) != 0) {
    return tl::unexpected(error::config_file_parse);
  } else {
    return cd;
  }
}