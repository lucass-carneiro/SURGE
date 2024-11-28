#include "sc_allocators.hpp"
#include "sc_cli.hpp"
#include "sc_config.hpp"
#include "sc_options.hpp"
#include "sc_tasks.hpp"

#include <cstdio>
#include <exception>

// Avoid using integrated graphics
#ifdef SURGE_SYSTEM_Windows
#  include <windows.h>
extern "C" {
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}
#endif

int main() {
  using namespace surge;
  using std::printf;

  try {
    // Avoid using integrated graphics on NV hardware.
    // TODO: Set this for AMD hardware
#ifdef SURGE_SYSTEM_Linux
    setenv("__NV_PRIME_RENDER_OFFLOAD", "1", 0);
    setenv("__GLX_VENDOR_LIBRARY_NAME", "nvidia", 0);
#endif

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
    const auto config_data{config::parse_config()};
    if (!config_data) {
      return EXIT_FAILURE;
    }

    const auto [w_res, w_ccl, w_attrs, r_attrs, first_mod] = *config_data;

    return EXIT_SUCCESS;
  } catch (const std::exception &e) {
    printf("FATAL: Unhandled exception caught: %s", e.what());
    return EXIT_FAILURE;
  }
}