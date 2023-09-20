#include "allocators.hpp"
#include "cli.hpp"
#include "config.hpp"
#include "files.hpp"
#include "logging.hpp"
#include "module.hpp"
#include "options.hpp"
#include "renderer.hpp"
#include "timers.hpp"
#include "window.hpp"

#include <cstdlib>

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
  auto window{window::init(w_res, w_attrs)};
  if (!window) {
    return EXIT_FAILURE;
  }

  /*********************
   * Load First module *
   *********************/
  if (m_list.size() == 0) {
    log_error("No module to initialize");
    return EXIT_FAILURE;
  }

  const auto &first_mod_name{m_list[0].c_str()};

  auto mod{module::load(first_mod_name)};
  if (!mod) {
    log_error("Unable to load first module %s", first_mod_name);
    return EXIT_FAILURE;
  }

  auto mod_api{module::get_api(*mod)};
  if (!mod_api) {
    log_error("Unable to recover first module %s API", first_mod_name);
    return EXIT_FAILURE;
  }

  const auto on_load_result{mod_api->on_load(*window)};
  if (on_load_result != 0) {
    log_error("Mudule %p returned error %i while calling on_load", *mod, on_load_result);
    module::unload(*mod);
    return EXIT_FAILURE;
  }

  /***********************
   * Main Loop variables *
   ***********************/
  timers::generic_timer frame_timer;
  timers::generic_timer update_timer;
  update_timer.start();

#ifdef SURGE_ENABLE_HR
  auto hr_key_old_state{glfwGetKey(*window, GLFW_KEY_F5)
                        && glfwGetKey(*window, GLFW_KEY_LEFT_CONTROL)};
#endif

  /*************
   * Main Loop *
   *************/
  while ((frame_timer.start(), !glfwWindowShouldClose(*window))) {
    glfwPollEvents();

    // Handle hot reloading
#ifdef SURGE_ENABLE_HR
    const auto should_hr{glfwGetKey(*window, GLFW_KEY_F5) == GLFW_PRESS
                         && glfwGetKey(*window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS
                         && hr_key_old_state == GLFW_RELEASE};
    if (should_hr) {
      timers::generic_timer t;
      t.start();

      mod_api->on_unload(*window);

      mod = module::reload(*mod);
      if (!mod) {
        break;
      }

      mod_api = module::get_api(*mod);
      if (!mod_api) {
        break;
      }

      const auto on_load_result{mod_api->on_load(*window)};
      if (on_load_result != 0) {
        log_error("Mudule %p returned error %i while calling on_load", *mod, on_load_result);
        break;
      }

      t.stop();
      log_info("Hot reloading succsesfull in %f s", t.elapsed());
    }
#endif

    // Call module update
    mod_api->update(update_timer.stop());
    update_timer.start();

    // Clear buffers
    renderer::clear(w_ccl);

    // Call module draw
    mod_api->draw();

    // Present rendering
    glfwSwapBuffers(*window);

    // Cache refresh key state
#ifdef SURGE_ENABLE_HR
    hr_key_old_state
        = glfwGetKey(*window, GLFW_KEY_F5) && glfwGetKey(*window, GLFW_KEY_LEFT_CONTROL);
#endif

    frame_timer.stop();
  }

  // Finalize modules
  mod_api->on_unload(*window);
  module::unload(*mod);

  // Finalize window and renderer
  window::terminate(*window);

  return EXIT_SUCCESS;
}