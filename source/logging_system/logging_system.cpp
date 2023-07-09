#include "logging_system/logging_system.hpp"

#include "options.hpp"

#ifdef SURGE_USE_LOG_COLOR
#  include <spdlog/sinks/stdout_color_sinks.h>
#else
#  include "spdlog/sinks/stdout_sinks.h"
#endif

#include <cstdio>

#ifdef SURGE_USE_LOG_COLOR

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::shared_ptr<spdlog::logger> surge::logger::logger_ptr
    = spdlog::stdout_color_mt("surge_stdout_logger");

auto surge::logger::init() noexcept -> bool {
  using std::printf;

  try {
    logger_ptr->set_pattern("\033[38;2;70;130;180m[%m-%d-%Y %H:%M:%S] "
                            "\033[38;2;127;255;212m[thread %t] "
                            "\033[1m%^%l:%$ "
                            "\033[0m%v");

    return true;
  } catch (const std::exception &e) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    printf("Unable to initialize logging system: %s", e.what());
    return false;
  }
}

#else

std::shared_ptr<spdlog::logger> surge::logger::logger_ptr
    = spdlog::stdout_logger_mt("surge_stdout_logger");

auto surge::logger::init() noexcept -> bool {
  using std::printf;

  try {
    logger->set_pattern("[%m-%d-%Y %H:%M:%S] [thread %t] %^%l:%$ %v");

    return true;
  } catch (const std::exception &e) {
    printf("Unable to initialize logging system: %s", e.what());
    return false;
  }
}

#endif

extern "C" void info(const char *msg) noexcept {
  using std::printf;

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  printf("\033[36m[thread %lu]\033[m \033[1m\033[35mSURGE VM Info:\033[m %s\n",
         std::hash<std::thread::id>{}(std::this_thread::get_id()), msg);
}

extern "C" void error(const char *msg) noexcept {
  using std::printf;

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  printf("\033[1m\033[31mSURGE VM Error:\033[m %s\n", msg);
}

extern "C" void warn(const char *msg) noexcept {
  using std::printf;

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  printf("\033[1m\033[33mSURGE VM Warning:\033[m %s\n", msg);
}

extern "C" void debug(const char *msg) noexcept {
  using std::printf;

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  printf("\033[1m\033[34mSURGE VM Debug:\033[m %s\n", msg);
}
