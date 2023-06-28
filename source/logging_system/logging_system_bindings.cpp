#include "logging_system/logging_system_bindings.hpp"

#include "logging_system/logging_system.hpp"

#include <cstdio>

extern "C" void log_info(const char *msg) noexcept {
  try {
    surge::log_info("{}", msg);
  } catch (std::exception &e) {
    printf("Unable to log info message %s using spdlog", msg);
  }
}

extern "C" void log_error(const char *msg) noexcept {
  try {
    surge::log_error("{}", msg);
  } catch (std::exception &e) {
    printf("Unable to log error message %s using spdlog", msg);
  }
}

extern "C" void log_warn(const char *msg) noexcept {
  try {
    surge::log_warn("{}", msg);
  } catch (std::exception &e) {
    printf("Unable to log warning message %s using spdlog", msg);
  }
}

extern "C" void log_debug(const char *msg) noexcept {
  try {
    surge::log_debug("{}", msg);
  } catch (std::exception &e) {
    printf("Unable to log debug message %s using spdlog", msg);
  }
}
