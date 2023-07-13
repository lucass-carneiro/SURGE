#ifndef SURGE_LOGGING_SYSTEM_H
#define SURGE_LOGGING_SYSTEM_H

#include "options.hpp"

#include <fmt/core.h>

#ifdef SURGE_SYSTEM_IS_POSIX
#  include <unistd.h>
#  define SURGE_TID_FUNCTION gettid()
#else
#  include <windows.h>
#  define SURGE_TID_FUNCTION GetCurrentThreadId()
#endif

#ifdef SURGE_USE_LOG_COLOR
#  define SURGE_ERROR_BANNER "\033[36m[Thread ID: {}]\033[m \033[1m\033[31mSURGE Error:\033[m "
#  define SURGE_INFO_BANNER "\033[36m[Thread ID: {}]\033[m \033[1m\033[32mSURGE Info:\033[m "
#  define SURGE_WARN_BANNER "\033[36m[Thread ID: {}]\033[m \033[1m\033[33mSURGE Warning:\033[m "
#else
#  define SURGE_ERROR_BANNER "[Thread ID: {}] SURGE Error: "
#  define SURGE_INFO_BANNER "[Thread ID: {}] SURGE Info: "
#  define SURGE_WARN_BANNER "[Thread ID: {}] SURGE Warning: "
#endif

#define log_error(format, ...)                                                                     \
  surge::logging::print(SURGE_ERROR_BANNER format "\n",                                            \
                        SURGE_TID_FUNCTION __VA_OPT__(, ) __VA_ARGS__)

#define log_info(format, ...)                                                                      \
  surge::logging::print(SURGE_INFO_BANNER format "\n",                                             \
                        SURGE_TID_FUNCTION __VA_OPT__(, ) __VA_ARGS__)

#define log_warn(format, ...)                                                                      \
  surge::logging::print(SURGE_WARN_BANNER format "\n",                                             \
                        SURGE_TID_FUNCTION __VA_OPT__(, ) __VA_ARGS__)

namespace surge::logging {

template <typename... T> void print(fmt::format_string<T...> fmt, T &&...args) noexcept {
  using std::printf;
  try {
    fmt::print(fmt, std::forward<T>(args)...);
  } catch (const std::exception &e) {
    printf("Unable to print using fmt: %s.\n", e.what());
  }
}

} // namespace surge::logging

#endif // SURGE_LOGGING_SYSTEM_H