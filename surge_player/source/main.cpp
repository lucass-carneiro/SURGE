#include "allocators.hpp"
#include "cli.hpp"
#include "files.hpp"
#include "font_cache.hpp"
#include "logging.hpp"
#include "module_manager.hpp"
#include "options.hpp"
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
  auto [window, ww, wh, ccl] = window::init("config.yaml");
  if (!window) {
    return EXIT_FAILURE;
  }

  /*******************
   * Init font cache *
   *******************/
  auto freetype_ctx{fonts::init(window, "config.yaml")};
  if (!freetype_ctx) {
    return EXIT_FAILURE;
  }

  auto char_map{fonts::create_character_maps(*freetype_ctx, 48)};
  if (!char_map) {
    return EXIT_FAILURE;
  }

  /*********************
   * Load First module *
   *********************/
  auto curr_module{module::load_first_module(argc, argv)};
  if (curr_module == nullptr) {
    window::terminate(window);
    return EXIT_FAILURE;
  }

  if (!module::on_load(window, curr_module)) {
    window::terminate(window);
    return EXIT_FAILURE;
  }

  /***********************
   * Main Loop variables *
   ***********************/
  timers::generic_timer frame_timer;
  timers::generic_timer update_timer;
  update_timer.start();

#ifdef SURGE_ENABLE_HR
  auto hr_key_old_state{glfwGetKey(window, GLFW_KEY_F5)
                        && glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)};
#endif

  /*************
   * Main Loop *
   *************/
  while ((frame_timer.start(), !glfwWindowShouldClose(window))) {
    glfwPollEvents();

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

    // Clear buffers
    renderer::clear(ccl);

    fonts::render_text(*freetype_ctx, *char_map, 0, glm::vec3{10.0f, 500.0f, 1.0f},
                       glm::vec3{220.0f / 256.0f, 20.0f / 256.0f, 60.0f / 256.0f},
                       "The quick brown fox jumps");
    fonts::render_text(*freetype_ctx, *char_map, 0, glm::vec3{10.0f, 450.0f, 1.0f},
                       glm::vec3{220.0f / 256.0f, 20.0f / 256.0f, 60.0f / 256.0f},
                       "over the lazy dog");

    fonts::render_text(*freetype_ctx, *char_map, 1, glm::vec3{10.0f, 350.0f, 1.0f},
                       glm::vec3{220.0f / 256.0f, 20.0f / 256.0f, 60.0f / 256.0f},
                       "The quick brown fox jumps");
    fonts::render_text(*freetype_ctx, *char_map, 1, glm::vec3{10.0f, 300.0f, 1.0f},
                       glm::vec3{220.0f / 256.0f, 20.0f / 256.0f, 60.0f / 256.0f},
                       "over the lazy dog");

    // Call module draw
    module::draw(curr_module);

    // Present rendering
    glfwSwapBuffers(window);

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

  // Finalize font cache
  fonts::terminate(*freetype_ctx);

  return EXIT_SUCCESS;
}