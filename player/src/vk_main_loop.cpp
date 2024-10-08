#include "player.hpp"

void player::vk_main_loop(surge::renderer::vk::context &ctx, opt_mod_handle &mod,
                          opt_mod_api &mod_api, int &on_load_result, opt_error &input_bind_result,
                          const surge::config::clear_color &w_ccl,
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

    // Acquire swapchain image
    const auto img_result{renderer::vk::request_img(ctx)};
    if (!img_result) {
      log_error("Vulkan error while acquiring swapchain images");
      break;
    }
    auto [img, img_idx] = *img_result;

    // Begin command recording
    {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
      ZoneScopedN("Cmd Begin");
#endif
      renderer::vk::cmd_begin(ctx);
    }

    // Clear screen
    {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
      ZoneScopedN("Clear");
#endif
      renderer::vk::clear(ctx, img, w_ccl);
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

    // End command recording
    {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
      ZoneScopedN("Cmd End");
#endif
      renderer::vk::cmd_end(ctx);
    }

    // Submit command buffer
    {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
      ZoneScopedN("Cmd Submit");
#endif
      renderer::vk::cmd_submit(ctx);
    }

    // Present
    {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
      ZoneScopedN("SWPC Present");
#endif
      const auto present_result{renderer::vk::present(ctx, img_idx)};
      if (present_result.has_value()) {
        log_error("Vulkan error while presenting swapchain images");
        break;
      }
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
#endif
  }
}