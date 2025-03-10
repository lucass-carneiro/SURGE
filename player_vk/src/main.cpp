#include "sc_allocators.hpp"
#include "sc_cli.hpp"
#include "sc_config.hpp"
#include "sc_options.hpp"
#include "sc_timers.hpp"

#ifdef SURGE_ENABLE_TRACY
#  include <tracy/Tracy.hpp>
#endif

#include <cstdio>
#include <exception>

int main() {
  using namespace surge;
  using std::printf;

#ifdef SURGE_ENABLE_TRACY
  ZoneScopedN("surge::main");
#endif

  try {
    /********
     * Logo *
     ********/
    cli::draw_logo();

    /*******************
     * Init allocators *
     *******************/
    allocators::mimalloc::init();
    allocators::scoped::init();

    /*********************
     * Parse config file *
     *********************/
    const auto config_data{config::parse_config(config::RenderBackend::vulkan)};
    if (!config_data) {
      return EXIT_FAILURE;
    }

    /***********************
     * Main Loop variables *
     ***********************/
    timers::GenericTimer frame_timer{};
    timers::GenericTimer update_timer{};
    update_timer.start();

    /*************
     * Main Loop *
     *************/
    while ((frame_timer.start(), false)) {

      // Stop frame timer
      frame_timer.stop();

      // Reset frame arena
      allocators::scoped::reset(allocators::scoped::Lifetimes::Frame);

#ifdef SURGE_ENABLE_TRACY
      FrameMark;
#endif
    }

    allocators::scoped::reset(allocators::scoped::Lifetimes::Program);
    allocators::scoped::destroy();

    return EXIT_SUCCESS;

  } catch (const std::exception &e) {
    printf("SURGE FATAL ERROR: Unhandled exception caught: %s\nB", e.what());
    return EXIT_FAILURE;
  }
}