#ifndef SURGE_LOGGING_HPP
#define SURGE_LOGGING_HPP

#include "options.hpp"

#ifdef SURGE_SYSTEM_IS_POSIX
#  include <unistd.h>
#  define SURGE_TID_FUNCTION gettid()
#else
#  include <windows.h>
#  define SURGE_TID_FUNCTION GetCurrentThreadId()
#endif

#include <cstdio>

#if defined(SURGE_USE_LOG_COLOR) && !defined(SURGE_SYSTEM_IS_POSIX)
#  define SURGE_ERROR_BANNER "\033[36m[Thread ID: %lu]\033[m \033[1m\033[31mSURGE Error:\033[m "
#  define SURGE_INFO_BANNER "\033[36m[Thread ID: %lu]\033[m \033[1m\033[32mSURGE Info:\033[m "
#  define SURGE_WARN_BANNER "\033[36m[Thread ID: %lu]\033[m \033[1m\033[33mSURGE Warning:\033[m "
#elif defined(SURGE_USE_LOG_COLOR) && defined(SURGE_SYSTEM_IS_POSIX)
#  define SURGE_ERROR_BANNER "\033[36m[Thread ID: %i]\033[m \033[1m\033[31mSURGE Error:\033[m "
#  define SURGE_INFO_BANNER "\033[36m[Thread ID: %i]\033[m \033[1m\033[32mSURGE Info:\033[m "
#  define SURGE_WARN_BANNER "\033[36m[Thread ID: %i]\033[m \033[1m\033[33mSURGE Warning:\033[m "
#else
#  define SURGE_ERROR_BANNER "[Thread ID: %i] SURGE Error: "
#  define SURGE_INFO_BANNER "[Thread ID: %i] SURGE Info: "
#  define SURGE_WARN_BANNER "[Thread ID: %i] SURGE Warning: "
#endif

#define log_error(format, ...)                                                                     \
  std::printf(SURGE_ERROR_BANNER format "\n", SURGE_TID_FUNCTION __VA_OPT__(, ) __VA_ARGS__)

#define log_info(format, ...)                                                                      \
  std::printf(SURGE_INFO_BANNER format "\n", SURGE_TID_FUNCTION __VA_OPT__(, ) __VA_ARGS__)

#define log_warn(format, ...)                                                                      \
  std::printf(SURGE_WARN_BANNER format "\n", SURGE_TID_FUNCTION __VA_OPT__(, ) __VA_ARGS__)

#endif // SURGE_LOGGING_HPP