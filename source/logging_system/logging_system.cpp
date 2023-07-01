#include "logging_system/logging_system.hpp"

#include <cstdio>

extern "C" void info(const char *msg) noexcept {
  try {
    surge::log_info("{}", msg);
  } catch (std::exception &e) {
    printf("Unable to log info message %s using spdlog", msg);
  }
}

extern "C" void error(const char *msg) noexcept {
  try {
    surge::log_error("{}", msg);
  } catch (std::exception &e) {
    printf("Unable to log error message %s using spdlog", msg);
  }
}

extern "C" void warn(const char *msg) noexcept {
  try {
    surge::log_warn("{}", msg);
  } catch (std::exception &e) {
    printf("Unable to log warning message %s using spdlog", msg);
  }
}

extern "C" void debug(const char *msg) noexcept {
  try {
    surge::log_debug("{}", msg);
  } catch (std::exception &e) {
    printf("Unable to log debug message %s using spdlog", msg);
  }
}
