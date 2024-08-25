#include "surge_core.hpp"

#if defined(SURGE_BUILD_TYPE_Profile)                                                              \
    || defined(SURGE_BUILD_TYPE_RelWithDebInfo) && defined(SURGE_ENABLE_TRACY)
#  include <common/TracyColor.hpp>
#  include <tracy/Tracy.hpp>
#  include <tracy/TracyOpenGL.hpp>
#endif

#include <atomic>
#include <thread>

// Avoid using integrated graphics
#ifdef SURGE_SYSTEM_Windows
extern "C" {
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}
#endif

using opt_mod_handle = tl::expected<surge::module::handle_t, surge::error>;
using opt_mod_api = tl::expected<surge::module::api, surge::error>;
using opt_error = std::optional<surge::error>;

static inline void gl_main_loop(opt_mod_handle &mod, opt_mod_api &mod_api, int &on_load_result,
                                opt_error &input_bind_result,
                                const surge::config::clear_color &w_ccl,
                                const surge::config::renderer_attrs &r_attrs) {
  using namespace surge;

  timers::generic_timer frame_timer;
  timers::generic_timer update_timer;
  update_timer.start();

#ifdef SURGE_ENABLE_HR
  auto hr_key_old_state{window::get_key(GLFW_KEY_F5) && window::get_key(GLFW_KEY_LEFT_CONTROL)};
#endif

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
    renderer::gl::clear(w_ccl);

    // Call module update
    if (mod_api->update(update_timer.stop()) != 0) {
      window::set_should_close(true);
    }
    update_timer.start();

    // Call module draw
    mod_api->draw();

    // Present rendering
    renderer::gl::wait_idle();
    window::swap_buffers();

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

static void vk_render_thread_loop(const surge::config::renderer_attrs &r_attrs,
                                  const surge::config::window_resolution &w_res,
                                  const surge::config::window_attrs &w_attrs,
                                  const surge::config::clear_color &w_ccl,
                                  std::atomic<bool> *done) noexcept {
  using namespace surge;

  log_info("Initializing Vulkan rendering thread");

  auto ctx{renderer::vk::initialize(r_attrs, w_res, w_attrs)};
  if (!ctx) {
    log_error("Unable to initialize Vulkan");
    return;
  }

  while (!*done) {
    surge::renderer::vk::clear(*ctx, w_ccl);
  }

  log_info("Finalizing Vulkan rendering thread");
  renderer::vk::terminate(*ctx);
}

static inline void vk_main_loop() noexcept {
  using namespace surge;

  while (!window::should_close()) {
    window::poll_events();
  }
}

auto main(int, char **) noexcept -> int {
  using namespace surge;

#if defined(SURGE_BUILD_TYPE_Profile)                                                              \
    || defined(SURGE_BUILD_TYPE_RelWithDebInfo) && defined(SURGE_ENABLE_TRACY)
  tracy::StartupProfiler();
#endif

  // Avoid using integrated graphics on NV hardware.
  // TODO: Set this for AMD hardware
#ifdef SURGE_SYSTEM_Linux
  setenv("__NV_PRIME_RENDER_OFFLOAD", "1", 0);
  setenv("__GLX_VENDOR_LIBRARY_NAME", "nvidia", 0);
#endif

#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
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

  const auto [w_res, w_ccl, w_attrs, r_attrs, first_mod] = *config_data;

  /***************
   * Init window *
   ***************/
  const auto window_init_result{window::init(w_res, w_attrs, r_attrs)};
  if (window_init_result.has_value()) {
    return EXIT_FAILURE;
  }

  /***********************
   * Init render backend *
   ***********************/
  std::atomic<bool> render_done{false};
  std::thread vulkan_render_thread;

  if (r_attrs.backend == config::renderer_backend::opengl) {
    if (renderer::gl::init(r_attrs).has_value()) {
      window::terminate();
      return EXIT_FAILURE;
    }
  } else {
    vulkan_render_thread
        = std::thread{vk_render_thread_loop, r_attrs, w_res, w_attrs, w_ccl, &render_done};
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

  auto on_load_result{mod_api->on_load()};
  if (on_load_result != 0) {
    log_error("Mudule {} returned error {} while calling on_load", static_cast<void *>(*mod),
              on_load_result);
    window::terminate();
    module::unload(*mod);
    return EXIT_FAILURE;
  }

  auto input_bind_result{window::bind_module_input_callbacks(&(mod_api.value()))};
  if (input_bind_result.has_value()) {
    window::terminate();
    module::unload(*mod);
    return EXIT_FAILURE;
  }

  if (r_attrs.backend == config::renderer_backend::opengl) {
    gl_main_loop(mod, mod_api, on_load_result, input_bind_result, w_ccl, r_attrs);
  } else {
    vk_main_loop();
  }

  // Finalize modules
  mod_api->on_unload();
  window::unbind_input_callbacks();
  module::unload(*mod);

  // Finalize window and renderer
  render_done = true;

  if (r_attrs.backend == config::renderer_backend::opengl) {
    renderer::gl::wait_idle();
  } else {
    vulkan_render_thread.join();
  }

  window::terminate();

#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
  log_info("Tracy may still be collecting profiling data. Please wait...");
  tracy::ShutdownProfiler();
#endif

  return EXIT_SUCCESS;
}
