#include "default.hpp"

#include "debug_window.hpp"
#include "logging.hpp"

extern "C" SURGE_MODULE_EXPORT auto on_load() noexcept -> std::uint32_t {
  log_info("Hello from module default");
  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto on_unload() noexcept -> std::uint32_t {
  log_info("Bye from module default");
  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto draw() noexcept -> std::uint32_t {
  mod_default::debug_window::draw();
  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto update(double) noexcept -> std::uint32_t {
  // Do nothing
  // log_info("dt = %.16f", dt);
  return 0;
}