#ifndef SURGE_MODULE_DEFAULT
#define SURGE_MODULE_DEFAULT

#include "options.hpp"

#if defined(SURGE_COMPILER_Clang) || defined(SURGE_COMPILER_GCC) && COMPILING_SURGE_MODULE_DEFAULT
#  define SURGE_MODULE_EXPORT __attribute__((__visibility__("default")))
#elif defined(SURGE_COMPILER_MSVC) && COMPILING_SURGE_MODULE_DEFAULT
#  define SURGE_MODULE_EXPORT __declspec(dllexport)
#elif defined(SURGE_COMPILER_MSVC)
#  define SURGE_MODULE_EXPORT __declspec(dllimport)
#else
#  define SURGE_MODULE_EXPORT
#endif

extern "C" {
SURGE_MODULE_EXPORT auto on_load() noexcept -> bool;
SURGE_MODULE_EXPORT void on_unload() noexcept;
}

#endif // SURGE_MODULE_DEFAULT