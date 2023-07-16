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
SURGE_MODULE_EXPORT void on_load() noexcept;
SURGE_MODULE_EXPORT void on_unload() noexcept;

SURGE_MODULE_EXPORT void draw() noexcept;
SURGE_MODULE_EXPORT void update(double dt) noexcept;

SURGE_MODULE_EXPORT void keyboard_event(int key, int action, int mods) noexcept;
SURGE_MODULE_EXPORT void mouse_button_event(int button, int action, int mods) noexcept;
SURGE_MODULE_EXPORT void mouse_scroll_event(double xoffset, double yoffset) noexcept;
}

#endif // SURGE_MODULE_DEFAULT