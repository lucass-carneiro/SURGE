#include "default.hpp"
#include "logging.hpp"

extern "C" SURGE_MODULE_EXPORT auto on_load() noexcept -> bool {
  log_info("Hello from module default");
  return true;
}

extern "C" SURGE_MODULE_EXPORT void on_unload() noexcept { log_info("Bye from module default"); }