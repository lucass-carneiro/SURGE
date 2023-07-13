#include "logging_system/logging_system.hpp"

#include <cstdio>

#ifdef SURGE_USE_LOG_COLOR
#  define SURGE_VM_ERROR_BANNER "\033[36m[Thread ID: %i]\033[m \033[1m\033[31mSURGE Error:\033[m "
#  define SURGE_VM_INFO_BANNER "\033[36m[Thread ID: %i]\033[m \033[1m\033[35mSURGE Info:\033[m "
#  define SURGE_VM_WARN_BANNER "\033[36m[Thread ID: %i]\033[m \033[1m\033[33mSURGE Warning:\033[m "
#else
#  define SURGE_VM_ERROR_BANNER "[Thread ID: %i] SURGE Error: "
#  define SURGE_VM_INFO_BANNER "[Thread ID: %i] SURGE Info: "
#  define SURGE_VM_WARN_BANNER "[Thread ID: %i] SURGE Warning: "
#endif

extern "C" void error(const char *msg) noexcept {
  using std::printf;
  printf(SURGE_VM_ERROR_BANNER "%s\n", SURGE_TID_FUNCTION, msg);
}

extern "C" void info(const char *msg) noexcept {
  using std::printf;
  printf(SURGE_VM_INFO_BANNER "%s\n", SURGE_TID_FUNCTION, msg);
}

extern "C" void warn(const char *msg) noexcept {
  using std::printf;
  printf(SURGE_VM_WARN_BANNER "%s\n", SURGE_TID_FUNCTION, msg);
}
