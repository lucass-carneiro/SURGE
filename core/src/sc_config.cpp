#include "sc_config.hpp"

#include "sc_allocators.hpp"
#include "sc_files.hpp"
#include "sc_logging.hpp"

#include <array>
#include <cstring>
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

auto surge::config::parse_config() -> tl::expected<config_data, error> {
  using std::atof;
  using std::atoi;

  auto config_file{files::load_file("config.yaml", true)};

  if (!config_file) {
    log_error("Unable to load config.yaml file");
    return tl::unexpected(error::config_file_load);
  }

  config_data cd{};
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

    cd.wattrs.name
        = string(tree["window"]["name"].val().data(), tree["window"]["name"].val().size());
    cd.wattrs.monitor_index = atoi(tree["window"]["monitor_index"].val().data());
    cd.wattrs.windowed = static_cast<bool>(atoi(tree["window"]["windowed"].val().data()));
    cd.wattrs.cursor = static_cast<bool>(atoi(tree["window"]["windowed"].val().data()));

    cd.rattrs.backend = atoi(tree["renderer"]["backend"].val().data()) == 0
                            ? renderer_backend::opengl
                            : renderer_backend::vulkan;
    cd.rattrs.vsync = static_cast<bool>(atoi(tree["renderer"]["VSync"].val().data()));
    cd.rattrs.MSAA = static_cast<bool>(atoi(tree["renderer"]["MSAA"].val().data()));
    cd.rattrs.fps_cap = static_cast<bool>(atoi(tree["renderer"]["fps_cap"].val().data()));
    cd.rattrs.fps_cap_value = atoi(tree["renderer"]["fps_cap_value"].val().data());

    cd.module = string(tree["modules"]["first_module"].val().data(),
                       tree["modules"]["first_module"].val().size());

    return cd;
  } catch (const std::exception &) {
    return tl::unexpected{error::config_file_parse};
  }
}