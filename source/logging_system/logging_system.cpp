#include "logging_system/logging_system.hpp"

#include <cstdio>

#ifdef SURGE_USE_LOG_COLOR

surge::log_manager::log_manager() : logger{spdlog::stdout_color_mt("surge_stdout_logger")} {
  logger->set_pattern("\033[38;2;70;130;180m[%m-%d-%Y %H:%M:%S] "
                      "\033[38;2;127;255;212m[thread %t] "
                      "\033[1m%^%l:%$ "
                      "\033[0m%v");
}

#else

surge::log_manager::log_manager() : logger{spdlog::stdout_logger_mt("surge_stdout_logger")} {
  logger->set_pattern("[%m-%d-%Y %H:%M:%S] [thread %t] %^%l:%$ %v");
}

#endif

extern "C" void info(const char *msg) noexcept {
  using std::printf;

  try {
    surge::log_info("{}", msg);
  } catch (const std::exception &e) {

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    printf("Unable to log info message %s using spdlog", msg);
  }
}

extern "C" void error(const char *msg) noexcept {
  using std::printf;

  try {
    surge::log_error("{}", msg);
  } catch (const std::exception &e) {

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    printf("Unable to log error message %s using spdlog", msg);
  }
}

extern "C" void warn(const char *msg) noexcept {
  using std::printf;

  try {
    surge::log_warn("{}", msg);
  } catch (const std::exception &e) {

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    printf("Unable to log warning message %s using spdlog", msg);
  }
}

extern "C" void debug(const char *msg) noexcept {
  using std::printf;

  try {
    surge::log_debug("{}", msg);
  } catch (const std::exception &e) {

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    printf("Unable to log debug message %s using spdlog", msg);
  }
}
