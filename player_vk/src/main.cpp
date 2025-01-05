#include "sc_allocators.hpp"
#include "sc_cli.hpp"
#include "sc_config.hpp"
#include "sc_logging.hpp"
#include "sc_module.hpp"
#include "sc_options.hpp"
#include "sc_tasks.hpp"
#include "sc_timers.hpp"
#include "sc_vulkan/sc_vulkan.hpp"
#include "sc_window.hpp"

#include <cstdio>
#include <exception>

#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#endif

int main() {
  using namespace surge;
  using std::printf;

  try {
#if defined(SURGE_BUILD_TYPE_Profile)                                                              \
    || defined(SURGE_BUILD_TYPE_RelWithDebInfo) && defined(SURGE_ENABLE_TRACY)
    tracy::StartupProfiler();
#endif

#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
    ZoneScopedN("Main");
#endif

    /*******************
     * Init allocators *
     *******************/
    allocators::mimalloc::init();

    /**********************
     * Init Task executor *
     **********************/
    tasks::executor::get();

    /********
     * Logo *
     ********/
    cli::draw_logo();

    /*********************
     * Parse config file *
     *********************/
    const auto config_data{config::parse_config(config::renderer_backend::vulkan)};
    if (!config_data) {
      return EXIT_FAILURE;
    }

    const auto &[w_res, w_ccl, w_attrs, r_attrs, first_mod] = *config_data;

    /***************
     * Init window *
     ***************/
    const auto engine_window{window::init(w_res, w_attrs, r_attrs)};
    if (!engine_window) {
      return EXIT_FAILURE;
    }

    /***********************
     * Init render backend *
     ***********************/
    auto vk_ctx{renderer::vk::initialize(*engine_window, r_attrs, w_res, w_attrs)};
    if (!vk_ctx) {
      window::terminate(*engine_window);
      return EXIT_FAILURE;
    }

    /*********************
     * Load First module *
     *********************/
    const auto &first_mod_name{first_mod.c_str()};

    if (!module::set_module_path()) {
      log_error("Unable to set the module path");
      renderer::vk::terminate(*vk_ctx);
      window::terminate(*engine_window);
      return EXIT_FAILURE;
    }

    auto mod{module::load(first_mod_name)};
    if (!mod) {
      log_error("Unable to load first module {}", first_mod_name);
      renderer::vk::terminate(*vk_ctx);
      window::terminate(*engine_window);
      return EXIT_FAILURE;
    }

    auto mod_api{module::get_vk_api(*mod)};
    if (!mod_api) {
      log_error("Unable to recover first module {} API", first_mod_name);
      renderer::vk::terminate(*vk_ctx);
      window::terminate(*engine_window);
      module::unload(*mod);
      return EXIT_FAILURE;
    }

    auto on_load_result{mod_api->on_load(*engine_window, *vk_ctx)};
    if (on_load_result != 0) {
      log_error("Mudule {} returned error {} while calling on_load", static_cast<void *>(*mod),
                on_load_result);
      renderer::vk::terminate(*vk_ctx);
      window::terminate(*engine_window);
      module::unload(*mod);
      return EXIT_FAILURE;
    }

    module::bind_input_callbacks(*engine_window, *mod, *mod_api);

    /***********************
     * Main Loop variables *
     ***********************/
    timers::generic_timer frame_timer;
    timers::generic_timer update_timer;
    update_timer.start();

#ifdef SURGE_ENABLE_HR
    auto hr_key_old_state{window::get_key(*engine_window, GLFW_KEY_F5)
                          && window::get_key(*engine_window, GLFW_KEY_LEFT_CONTROL)};
#endif

    /*************
     * Main Loop *
     *************/
    while ((frame_timer.start(), !window::should_close(*engine_window))) {
      // Event handling
      window::poll_events();

      // Handle hot reloading
#ifdef SURGE_ENABLE_HR
      const auto should_hr{window::get_key(*engine_window, GLFW_KEY_F5) == GLFW_PRESS
                           && window::get_key(*engine_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS
                           && hr_key_old_state == GLFW_RELEASE};
      if (should_hr) {
        timers::generic_timer t;
        t.start();

        module::unbind_input_callbacks(*engine_window);
        mod_api->on_unload(*engine_window, *vk_ctx);

        mod = module::reload(*mod);
        if (!mod) {
          break;
        }

        mod_api = module::get_vk_api(*mod);
        if (!mod_api) {
          break;
        }

        on_load_result = mod_api->on_load(*engine_window, *vk_ctx);
        if (on_load_result != 0) {
          log_error("Mudule {} returned error {} while calling on_load", static_cast<void *>(*mod),
                    on_load_result);
          break;
        }

        module::bind_input_callbacks(*engine_window, *mod, *mod_api);

        t.stop();
        log_info("Hot reloading succsesfull in {} s", t.elapsed());
      }
#endif

      // Acquire swapchain image
      const auto img_result{renderer::vk::request_swpc_img(*vk_ctx)};
      if (!img_result) {
        log_error("Vulkan error while acquiring swapchain images");
        break;
      }

      // Begin command recording
      {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
        ZoneScopedN("Cmd Begin");
#endif
        renderer::vk::cmd_begin(*vk_ctx);
      }

      // Clear screen
      {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
        ZoneScopedN("Clear");
#endif
        renderer::vk::clear_swpc(*vk_ctx, w_ccl);
      }

      // Call module update
      {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
        ZoneScopedN("Update");
#endif
        if (mod_api->update(*engine_window, *vk_ctx, update_timer.stop()) != 0) {
          window::set_should_close(*engine_window, true);
        }
      }
      update_timer.start();

      // Call module draw
      {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
        ZoneScopedN("Draw");
#endif
        mod_api->draw(*engine_window, *vk_ctx);
      }

      // End command recording
      {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
        ZoneScopedN("Cmd End");
#endif
        renderer::vk::cmd_end(*vk_ctx);
      }

      // Submit command buffer
      {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
        ZoneScopedN("Cmd Submit");
#endif
        renderer::vk::cmd_submit(*vk_ctx);
      }

      // Present
      {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
        ZoneScopedN("SWPC Present");
#endif
        const auto present_result{renderer::vk::present_swpc(*vk_ctx, r_attrs, w_res)};
        if (present_result.has_value()) {
          log_error("Vulkan error while presenting swapchain images");
          break;
        }
      }

      // Refresh HR key state
#ifdef SURGE_ENABLE_HR
      hr_key_old_state = window::get_key(*engine_window, GLFW_KEY_F5)
                         && window::get_key(*engine_window, GLFW_KEY_LEFT_CONTROL);
#endif

      // FPS Cap.
      if (r_attrs.fps_cap) {
        while (frame_timer.since_start() < (1.0 / r_attrs.fps_cap_value)) {
          // Spin wait
        }
      }

      frame_timer.stop();

#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
      FrameMark;
#endif
    }

    /********************
     * Finalize modules *
     ********************/
    module::unbind_input_callbacks(*engine_window);
    mod_api->on_unload(*engine_window, *vk_ctx);
    module::unload(*mod);

    /********************************
     * Finalize window and renderer *
     ********************************/
    renderer::vk::terminate(*vk_ctx);

    window::terminate(*engine_window);

#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
    log_info("Tracy may still be collecting profiling data. Please wait...");
    tracy::ShutdownProfiler();
#endif

    return EXIT_SUCCESS;
  } catch (const std::exception &e) {
    printf("FATAL: Unhandled exception caught: %s", e.what());
    return EXIT_FAILURE;
  }
}