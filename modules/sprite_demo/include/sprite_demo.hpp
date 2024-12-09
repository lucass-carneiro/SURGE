#ifndef SURGE_MODULE_SPRITE_DEMO
#define SURGE_MODULE_SPRITE_DEMO

#include "sc_glfw_includes.hpp"
#include "sc_options.hpp"

#if defined(SURGE_COMPILER_Clang)                                                                  \
    || defined(SURGE_COMPILER_GCC) && COMPILING_SURGE_MODULE_SPRITE_DEMO
#  define SURGE_MODULE_EXPORT __attribute__((__visibility__("default")))
#elif defined(SURGE_COMPILER_MSVC) && COMPILING_SURGE_MODULE_SPRITE_DEMO
#  define SURGE_MODULE_EXPORT __declspec(dllexport)
#elif defined(SURGE_COMPILER_MSVC)
#  define SURGE_MODULE_EXPORT __declspec(dllimport)
#else
#  define SURGE_MODULE_EXPORT
#endif

extern "C" {

SURGE_MODULE_EXPORT auto on_load(surge::window::window_t w) noexcept -> int;

SURGE_MODULE_EXPORT auto on_unload(surge::window::window_t w) noexcept -> int;

SURGE_MODULE_EXPORT auto draw(surge::window::window_t w) noexcept -> int;

SURGE_MODULE_EXPORT auto update(surge::window::window_t w, double dt) noexcept -> int;

SURGE_MODULE_EXPORT void keyboard_event(surge::window::window_t w, int key, int scancode,
                                        int action, int mods) noexcept;

SURGE_MODULE_EXPORT void mouse_button_event(surge::window::window_t w, int button, int action,
                                            int mods) noexcept;

SURGE_MODULE_EXPORT void mouse_scroll_event(surge::window::window_t w, double xoffset,
                                            double yoffset) noexcept;
}

#endif // SURGE_MODULE_SPRITE_DEMO