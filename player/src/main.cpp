#include "allocators.hpp"
#include "cli.hpp"
#include "config.hpp"
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

  const auto [w_res, w_ccl, w_attrs, first_mod] = *config_data;

  /***************
   * Init window *
   ***************/
  auto window{window::init(w_res, w_attrs)};
  if (!window) {
    return EXIT_FAILURE;
  }

  /*****************
   * Init Renderer *
   *****************/
  auto vk_ctx{renderer::init(w_attrs.name, *window)};
  if (!vk_ctx) {
    window::terminate(*window);
    return EXIT_FAILURE;
  }

  /***********************
   * Main Loop variables *
   ***********************/
  timers::generic_timer frame_timer;

  /*************
   * Main Loop *
   *************/
  while ((frame_timer.start(), !glfwWindowShouldClose(*window))) {
    glfwPollEvents();

    frame_timer.stop();
  }

  // Normal shutdown
  renderer::terminate(*vk_ctx);
  window::terminate(*window);
  return EXIT_SUCCESS;
}