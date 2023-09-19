#ifndef SURGE_MODULE_DEFAULT
#define SURGE_MODULE_DEFAULT

#include "options.hpp"

#include <cstdint>

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
SURGE_MODULE_EXPORT auto on_load() noexcept -> std::uint32_t;
SURGE_MODULE_EXPORT auto on_unload() noexcept -> std::uint32_t;
SURGE_MODULE_EXPORT auto draw() noexcept -> std::uint32_t;
SURGE_MODULE_EXPORT auto update(double dt) noexcept -> std::uint32_t;
}

#endif // SURGE_MODULE_DEFAULT