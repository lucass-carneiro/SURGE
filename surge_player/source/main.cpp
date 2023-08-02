#include "allocators.hpp"
#include "cli.hpp"
#include "files.hpp"
#include "logging.hpp"
#include "module_manager.hpp"
#include "options.hpp"
#include "surge_player.hpp"
#include "timers.hpp"
#include "window.hpp"

#include <GLFW/glfw3.h>
#include <cstdlib>

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

  /******************
   * Current module *
   ******************/
  auto curr_module{argc == 3 ? module::load(argv[2]) : module::load("./default.so")};
  if (curr_module == nullptr) {
    window::terminate(window);
    return EXIT_FAILURE;
  }

  if (!module::on_load(window, curr_module)) {
    window::terminate(window);
    return EXIT_FAILURE;
  }

  /*************
   * Main Loop *
   *************/
  timers::generic_timer frame_timer;
  timers::generic_timer update_timer;
  update_timer.start();

#ifdef SURGE_ENABLE_HR
  auto hr_key_old_state{glfwGetKey(window, GLFW_KEY_F5)
                        && glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)};
#endif

  while ((frame_timer.start(), !glfwWindowShouldClose(window))) {
    glfwPollEvents();

    // No need to do that, since we are creating non resizable windows, but good to have
    window::handle_resize(window, ww, wh, rf);

    // Handle hot reloading
#ifdef SURGE_ENABLE_HR
    const auto should_hr{glfwGetKey(window, GLFW_KEY_F5) == GLFW_PRESS
                         && glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS
                         && hr_key_old_state == GLFW_RELEASE};
    if (should_hr) {
      auto new_module = module::reload(window, curr_module);
      curr_module = new_module;
      if (!curr_module) {
        break;
      }
    }
#endif

    // Call module update
    module::update(curr_module, update_timer.stop());
    update_timer.start();

    bgfx::touch(0);

    // Call module draw
    module::draw(curr_module);

    // bgfx frame draw
    bgfx::frame();

    // Cache refresh key state
#ifdef SURGE_ENABLE_HR
    hr_key_old_state = glfwGetKey(window, GLFW_KEY_F5) && glfwGetKey(window, GLFW_KEY_LEFT_CONTROL);
#endif

    frame_timer.stop();
  }

  // Finalize modules
  module::on_unload(window, curr_module);
  module::unload(curr_module);

  // Finalize window and renderer
  window::terminate(window);

  return EXIT_SUCCESS;
}