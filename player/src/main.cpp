#include "allocators.hpp"
#include "cli.hpp"
#include "config.hpp"
#include "files.hpp"

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

  const auto [w_res, w_ccl, w_attrs] = *config_data;

  return EXIT_SUCCESS;
}