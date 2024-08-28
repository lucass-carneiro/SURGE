#include "player.hpp"

// Avoid using integrated graphics
#ifdef SURGE_SYSTEM_Windows
extern "C" {
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}
#endif

auto main(int, char **) -> int {

  // Avoid using integrated graphics on NV hardware.
  // TODO: Set this for AMD hardware
#ifdef SURGE_SYSTEM_Linux
  setenv("__NV_PRIME_RENDER_OFFLOAD", "1", 0);
  setenv("__GLX_VENDOR_LIBRARY_NAME", "nvidia", 0);
#endif

  using namespace surge;
  using namespace player;

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
  if (r_attrs.backend == config::renderer_backend::opengl) {
    log_info("Using OpenGL render backend.");

    if (renderer::gl::init(r_attrs).has_value()) {
      window::terminate();
      return EXIT_FAILURE;
    }
  } else {
    log_info("Using Vulkan render backend.");
    // TODO
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

  /*************
   * Main loop *
   *************/
  if (r_attrs.backend == config::renderer_backend::opengl) {
    gl_main_loop(mod, mod_api, on_load_result, input_bind_result, w_ccl, r_attrs);
  } else {
    // TODO
  }

  /********************
   * Finalize modules *
   ********************/
  mod_api->on_unload();
  window::unbind_input_callbacks();
  module::unload(*mod);

  /********************************
   * Finalize window and renderer *
   ********************************/
  if (r_attrs.backend == config::renderer_backend::opengl) {
    renderer::gl::wait_idle();
  } else {
    // TODO
  }

  window::terminate();

#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
  log_info("Tracy may still be collecting profiling data. Please wait...");
  tracy::ShutdownProfiler();
#endif

  return EXIT_SUCCESS;
}