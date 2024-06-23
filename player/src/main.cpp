#include "surge_core.hpp"

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
#  include <common/TracyColor.hpp>
#  include <tracy/Tracy.hpp>
#  include <tracy/TracyOpenGL.hpp>
#endif

auto main(int, char **) noexcept -> int {
  using namespace surge;

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("suge::main()");
#endif

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

  const auto [w_res, w_ccl, w_attrs, first_mod] = *config_data;

  /***************
   * Init window *
   ***************/
  auto window{window::init(w_res, w_attrs)};
  if (!window) {
    return EXIT_FAILURE;
  }

  // Finalize window and renderer
  window::terminate(*window);

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  log_info("Tracy may still be collecting profiling data. Please wait...");
#endif

  return EXIT_SUCCESS;
}
