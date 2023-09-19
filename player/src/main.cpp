#include "allocators.hpp"
#include "cli.hpp"
#include "config.hpp"
#include "files.hpp"
#include "logging.hpp"
#include "module.hpp"

auto main(int, char **) noexcept -> int {
  using namespace surge;

  /*******************
   * Init allocators *
   *******************/
  allocators::mimalloc::init();

  /********
   * Logo *
   ********/
  cli::draw_logo();

  /*********************
   * Parse config file *
   *********************/
  const auto config_data{config::parse_config()};
  if (!config_data) {
    return EXIT_FAILURE;
  }

  const auto [w_res, w_ccl, w_attrs, m_list] = *config_data;

  /****************************
   * Init window and renderer *
   ****************************/

  /*********************
   * Load First module *
   *********************/
  if (m_list.size() == 0) {
    log_error("No module to initialize");
    return EXIT_FAILURE;
  }

  const auto &first_mod_name{m_list[0].c_str()};

  auto mod{module::load(first_mod_name).value()};
  if (!mod) {
    log_error("Unable to load firs module %s", first_mod_name);
    return EXIT_FAILURE;
  }

  auto mod_api{module::get_api(mod)};
  if (!mod_api) {
    log_error("Unable to recover first module %s API", first_mod_name);
    return EXIT_FAILURE;
  }

  mod_api->on_load();
  mod_api->on_unload();
  module::unload(mod);

  return EXIT_SUCCESS;
}