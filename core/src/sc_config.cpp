#include "sc_config.hpp"

#include "sc_files.hpp"
#include "sc_logging.hpp"

#include <cstdlib>
#include <ryml/ryml.hpp>

static void ryml_error(const char *msg, size_t, ryml::Location location, void *) {
  log_error("Error while parsing config.yaml: {}\n"
            "  col: {}\n"
            "  line: {}\n"
            "  offset: {}",
            msg, location.col, location.line, location.offset);
  throw std::runtime_error("config.yaml parse error");
}

static auto ryml_alloc(size_t len, void *, void *) -> void * {
  return surge::allocators::mimalloc::malloc(len);
}

static void ryml_free(void *mem, size_t, void *) { surge::allocators::mimalloc::free(mem); }

auto surge::config::parse_config() -> Result<ConfigData> {
  using std::atoi;
  using std::strtof;

  auto config_file{files::as_bytes("config.yaml", true)};

  if (!config_file) {
    log_error("Unable to load config.yaml file");
    return Err{Error::config_file_load};
  }

  ConfigData cd{};
  const auto file_str{reinterpret_cast<const char *>(config_file->data())};

  try {
    const ryml::Callbacks callbacks{nullptr, &ryml_alloc, &ryml_free, &ryml_error};
    ryml::set_callbacks(callbacks);
    const auto tree{ryml::parse_in_arena(file_str)};

    cd.wr.width = atoi(tree["resolution"]["width"].val().data());
    cd.wr.height = atoi(tree["resolution"]["height"].val().data());

    cd.ccl.r = strtof(tree["clear_color"]["r"].val().data(), nullptr);
    cd.ccl.g = strtof(tree["clear_color"]["g"].val().data(), nullptr);
    cd.ccl.b = strtof(tree["clear_color"]["b"].val().data(), nullptr);
    cd.ccl.a = strtof(tree["clear_color"]["a"].val().data(), nullptr);

    cd.wattrs.name = ConfigData::String(tree["window"]["name"].val().data(),
                                        tree["window"]["name"].val().size());
    cd.wattrs.monitor_index = atoi(tree["window"]["monitor_index"].val().data());

    cd.wattrs.windowed = tree["window"]["windowed"].val() == "true";
    cd.wattrs.cursor = tree["window"]["cursor"].val() == "true";
    cd.wattrs.allow_resizes = tree["window"]["allow_resizes"].val() == "true";

    cd.rattrs.vsync = tree["renderer"]["VSync"].val() == "true";
    cd.rattrs.MSAA = tree["renderer"]["MSAA"].val() == "true";
    cd.rattrs.fps_cap = tree["renderer"]["fps_cap"].val() == "true";

    cd.rattrs.fps_cap_value = atoi(tree["renderer"]["fps_cap_value"].val().data());

    cd.module = ConfigData::String(tree["modules"]["first_module"].val().data(),
                                   tree["modules"]["first_module"].val().size());

    return cd;
  } catch (const std::exception &) {
    return tl::unexpected{Error::config_file_parse};
  }
}