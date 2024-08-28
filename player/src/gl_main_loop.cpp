#include "player.hpp"

void player::gl_main_loop(opt_mod_handle &mod, opt_mod_api &mod_api, int &on_load_result,
                          opt_error &input_bind_result, const surge::config::clear_color &w_ccl,
                          const surge::config::renderer_attrs &r_attrs) noexcept {
  using namespace surge;

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
      window::unbind_input_callbacks();

      mod = module::reload(*mod);
      if (!mod) {
        break;
      }

      mod_api = module::get_api(*mod);
      if (!mod_api) {
        break;
      }

      on_load_result = mod_api->on_load();
      if (on_load_result != 0) {
        log_error("Mudule {} returned error {} while calling on_load", static_cast<void *>(*mod),
                  on_load_result);
        break;
      }

      input_bind_result = window::bind_module_input_callbacks(&(mod_api.value()));
      if (input_bind_result.has_value()) {
        break;
      }

      t.stop();
      log_info("Hot reloading succsesfull in {} s", t.elapsed());
    }
#endif

    // Clear buffers
    {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
      ZoneScopedN("Clear");
#endif
      renderer::gl::clear(w_ccl);
    }

    // Call module update
    {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
      ZoneScopedN("Update");
#endif
      if (mod_api->update(update_timer.stop()) != 0) {
        window::set_should_close(true);
      }
    }
    update_timer.start();

    // Call module draw
    {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
      ZoneScopedN("Draw");
#endif
      mod_api->draw();
    }

    // Present rendering
    {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
      ZoneScopedN("Present");
#endif
      renderer::gl::wait_idle();
      window::swap_buffers();
    }

    // Refresh HR key state
#ifdef SURGE_ENABLE_HR
    hr_key_old_state = window::get_key(GLFW_KEY_F5) && window::get_key(GLFW_KEY_LEFT_CONTROL);
#endif

    frame_timer.stop();

// FPS Cap. On Linux, even with VSync this is necessary, otherwise the FPS may go above 60
#ifdef SURGE_SYSTEM_Linux
    if (r_attrs.fps_cap && (frame_timer.elapsed() < (1.0 / r_attrs.fps_cap_value))) {
      std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1>>(
          (1.0 / r_attrs.fps_cap_value) - frame_timer.elapsed()));
    }
#endif

#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
    FrameMark;
    TracyGpuCollect;
#endif
  }
}