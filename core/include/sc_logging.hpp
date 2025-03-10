#ifndef SURGE_CORE_LOGGING_HPP
#define SURGE_CORE_LOGGING_HPP

#include "sc_options.hpp"

#ifdef SURGE_SYSTEM_IS_POSIX
#  include <unistd.h>
#  define SURGE_TID_FUNCTION gettid()
#else
#  include <windows.h>
#  define SURGE_TID_FUNCTION GetCurrentThreadId()
#endif

#include <fmt/core.h>

#if defined(SURGE_USE_LOG_COLOR) && !defined(SURGE_SYSTEM_IS_POSIX)
#  define SURGE_ERROR_BANNER "\033[36m[Thread ID: {}]\033[m \033[1m\033[31mSURGE Error:\033[m "
#  define SURGE_INFO_BANNER "\033[36m[Thread ID: {}]\033[m \033[1m\033[32mSURGE Info:\033[m "
#  define SURGE_WARN_BANNER "\033[36m[Thread ID: {}]\033[m \033[1m\033[33mSURGE Warning:\033[m "
#  define SURGE_DEBUG_BANNER "\033[36m[Thread ID: {}]\033[m \033[1m\033[34mSURGE Debug Info:\033[m "
#elif defined(SURGE_USE_LOG_COLOR) && defined(SURGE_SYSTEM_IS_POSIX)
#  define SURGE_ERROR_BANNER "\033[36m[Thread ID: {}]\033[m \033[1m\033[31mSURGE Error:\033[m "
#  define SURGE_INFO_BANNER "\033[36m[Thread ID: {}]\033[m \033[1m\033[32mSURGE Info:\033[m "
#  define SURGE_WARN_BANNER "\033[36m[Thread ID: {}]\033[m \033[1m\033[33mSURGE Warning:\033[m "
#  define SURGE_DEBUG_BANNER "\033[36m[Thread ID: {}]\033[m \033[1m\033[34mSURGE Debug Info:\033[m "
#else
#  define SURGE_ERROR_BANNER "[Thread ID: {}] SURGE Error: "
#  define SURGE_INFO_BANNER "[Thread ID: {}] SURGE Info: "
#  define SURGE_WARN_BANNER "[Thread ID: {}] SURGE Warning: "
#  define SURGE_DEBUG_BANNER "[Thread ID: {}] SURGE Debug Info: "
#endif

#define log_error(format, ...)                                                                     \
  fmt::print(SURGE_ERROR_BANNER format "\n", SURGE_TID_FUNCTION __VA_OPT__(, ) __VA_ARGS__)

#define log_info(format, ...)                                                                      \
  fmt::print(SURGE_INFO_BANNER format "\n", SURGE_TID_FUNCTION __VA_OPT__(, ) __VA_ARGS__)

#define log_warn(format, ...)                                                                      \
  fmt::print(SURGE_WARN_BANNER format "\n", SURGE_TID_FUNCTION __VA_OPT__(, ) __VA_ARGS__)

#define log_debug(format, ...)                                                                     \
  fmt::print(SURGE_DEBUG_BANNER format "\n", SURGE_TID_FUNCTION __VA_OPT__(, ) __VA_ARGS__)

#endif // SURGE_CORE_LOGGING_HPP