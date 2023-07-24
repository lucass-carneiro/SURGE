#include "allocators.hpp"
#include "cli.hpp"
#include "files.hpp"
#include "module_manager.hpp"
#include "surge_player.hpp"
#include "timers.hpp"
#include "window.hpp"

auto main(int argc, char **argv) noexcept -> int {
  using namespace surge;

  /*******************
   * Init allocators *
   *******************/
  allocators::mimalloc::init();

  /********
   * Logo *
   ********/
  cli::draw_logo();

  /******************
   * Parse CLI args *
   ******************/
  if (!cli::parse_arguments(argc, argv)) {
    return EXIT_FAILURE;
  }

  /****************************
   * Init window and renderer *
   ****************************/
  auto [window, ww, wh, rf] = window::init(argv[1]);
  if (!window) {
    return EXIT_FAILURE;
  }

  /*************
   * Main Loop *
   *************/
  timers::generic_timer frame_timer;

  while ((frame_timer.start(), !glfwWindowShouldClose(window))) {
    glfwPollEvents();

    // No need to do that, since we are creating non resizable windows, but good to have
    // window::handle_resize(window, ww, wh, rf);

    bgfx::touch(0);

    bgfx::dbgTextClear();
    bgfx::setDebug(BGFX_DEBUG_STATS);

    bgfx::frame();

    frame_timer.stop();
  }

  window::terminate(window);

  return EXIT_SUCCESS;
}