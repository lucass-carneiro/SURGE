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

  auto curr_module{module::owned_module(m_list[0].c_str())};
  curr_module.on_load();
  /*if (curr_module == nullptr) {
    window::terminate(window);
    return EXIT_FAILURE;
  }

  if (!module::on_load(window, curr_module)) {
    window::terminate(window);
    return EXIT_FAILURE;
  }*/

  return EXIT_SUCCESS;
}