#ifndef SURGE_CORE_MODULE_TEXT_DEMO_HPP
#define SURGE_CORE_MODULE_TEXT_DEMO_HPP

#include "sc_glfw_includes.hpp"
#include "sc_options.hpp"

#if defined(SURGE_COMPILER_Clang) || defined(SURGE_COMPILER_GCC) && COMPILING_SURGE_MODULE_TEXT_DEMO
#  define SURGE_MODULE_EXPORT __attribute__((__visibility__("default")))
#elif defined(SURGE_COMPILER_MSVC) && COMPILING_SURGE_MODULE_TEXT_DEMO
#  define SURGE_MODULE_EXPORT __declspec(dllexport)
#elif defined(SURGE_COMPILER_MSVC)
#  define SURGE_MODULE_EXPORT __declspec(dllimport)
#else
#  define SURGE_MODULE_EXPORT
#endif

extern "C" {

SURGE_MODULE_EXPORT auto gl_on_load(surge::window::window_t w) -> int;

SURGE_MODULE_EXPORT auto gl_on_unload(surge::window::window_t w) -> int;

SURGE_MODULE_EXPORT auto gl_draw(surge::window::window_t w) -> int;

SURGE_MODULE_EXPORT auto gl_update(surge::window::window_t w, double dt) -> int;

SURGE_MODULE_EXPORT void gl_keyboard_event(surge::window::window_t w, int key, int scancode,
                                           int action, int mods);

SURGE_MODULE_EXPORT void gl_mouse_button_event(surge::window::window_t w, int button, int action,
                                               int mods);

SURGE_MODULE_EXPORT void gl_mouse_scroll_event(surge::window::window_t w, double xoffset,
                                               double yoffset);
}

#endif // SURGE_CORE_MODULE_TEXT_DEMO_HPP