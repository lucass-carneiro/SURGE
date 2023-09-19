#include "allocators.hpp"
#include "cli.hpp"
#include "config.hpp"
#include "files.hpp"
#include "logging.hpp"
#include "module.hpp"
#include "options.hpp"
#include "renderer.hpp"
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

  const auto on_load_result{mod_api->on_load()};
  if (on_load_result != 0) {
    log_error("Mudule %p returned error %i while calling on_load", *mod, on_load_result);
    module::unload(*mod);
    return EXIT_FAILURE;
  }

  /***********************
   * Main Loop variables *
   ***********************/
#ifdef SURGE_ENABLE_HR
  auto hr_key_old_state{glfwGetKey(*window, GLFW_KEY_F5)
                        && glfwGetKey(*window, GLFW_KEY_LEFT_CONTROL)};
#endif

  /*************
   * Main Loop *
   *************/
  while (!glfwWindowShouldClose(*window)) {
    glfwPollEvents();

    // Handle hot reloading
#ifdef SURGE_ENABLE_HR
    const auto should_hr{glfwGetKey(*window, GLFW_KEY_F5) == GLFW_PRESS
                         && glfwGetKey(*window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS
                         && hr_key_old_state == GLFW_RELEASE};
    if (should_hr) {
      log_info("HR");
      /*auto new_module = module::reload(window, curr_module);
      curr_module = new_module;
      if (!curr_module) {
        break;
      }*/
    }
#endif

    // Call module update
    mod_api->update();

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
  }

  mod_api->on_unload();
  module::unload(*mod);

  window::terminate(*window);

  return EXIT_SUCCESS;
}