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
  const auto window_init_result{window::init(w_res, w_attrs)};
  if (window_init_result.has_value()) {
    return EXIT_FAILURE;
  }

  /*********************
   * Load First module *
   *********************/
  const auto &first_mod_name{first_mod.c_str()};

  if (!module::set_module_path()) {
    log_error("Unable to set the module path");
    window::terminate();
    return EXIT_FAILURE;
  }

  auto mod{module::load(first_mod_name)};
  if (!mod) {
    log_error("Unable to load first module {}", first_mod_name);
    window::terminate();
    return EXIT_FAILURE;
  }

  auto mod_api{module::get_api(*mod)};
  if (!mod_api) {
    log_error("Unable to recover first module {} API", first_mod_name);
    window::terminate();
    module::unload(*mod);
    return EXIT_FAILURE;
  }

  const auto on_load_result{mod_api->on_load()};
  if (on_load_result != 0) {
    log_error("Mudule {} returned error {} while calling on_load", static_cast<void *>(*mod),
              on_load_result);
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
  auto hr_key_old_state{window::get_key(GLFW_KEY_F5) && window::get_key(GLFW_KEY_LEFT_CONTROL)};
#endif

  /*************
   * Main Loop *
   *************/
  while ((frame_timer.start(), !window::should_close())) {
    window::poll_events();

    // Handle hot reloading
#ifdef SURGE_ENABLE_HR
    const auto should_hr{window::get_key(GLFW_KEY_F5) == GLFW_PRESS
                         && window::get_key(GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS
                         && hr_key_old_state == GLFW_RELEASE};
    if (should_hr) {
      timers::generic_timer t;
      t.start();

      mod_api->on_unload();

      mod = module::reload(*mod);
      if (!mod) {
        break;
      }

      mod_api = module::get_api(*mod);
      if (!mod_api) {
        break;
      }

      const auto on_load_result{mod_api->on_load()};
      if (on_load_result != 0) {
        log_error("Mudule {} returned error {} while calling on_load", static_cast<void*>(*mod), on_load_result);
        break;
      }

      t.stop();
      log_info("Hot reloading succsesfull in {} s", t.elapsed());
    }
#endif

    // Call module update
    if (mod_api->update(update_timer.stop()) != 0) {
      window::set_should_close(true);
    }
    update_timer.start();

    // Clear buffers
    // TODO

    // Call module draw
    mod_api->draw();

    // Present rendering
    // TODO

    // Cache refresh key state
#ifdef SURGE_ENABLE_HR
    hr_key_old_state = window::get_key(GLFW_KEY_F5) && window::get_key(GLFW_KEY_LEFT_CONTROL);
#endif

    frame_timer.stop();

    // FPS Cap
    if (w_attrs.fps_cap && (frame_timer.elapsed() < (1.0 / w_attrs.fps_cap_value))) {
      std::this_thread::sleep_for(
          std::chrono::duration<double>((1.0 / w_attrs.fps_cap_value) - frame_timer.elapsed()));
    }

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
    FrameMark;
    TracyGpuCollect;
#endif
  }

  // Finalize modules
  mod_api->on_unload();
  module::unload(*mod);

  // Finalize window and renderer
  window::terminate();

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  log_info("Tracy may still be collecting profiling data. Please wait...");
#endif

  return EXIT_SUCCESS;
}
