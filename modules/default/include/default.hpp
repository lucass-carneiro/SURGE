#ifndef SURGE_CORE_MODULE_DEFAULT_HPP
#define SURGE_CORE_MODULE_DEFAULT_HPP

#include "sc_glfw_includes.hpp"
#include "sc_options.hpp"
#include "sc_vulkan/sc_vulkan.hpp"

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

SURGE_MODULE_EXPORT auto vk_on_load(surge::window::window_t w,
                                    surge::renderer::vk::context ctx) -> int;

SURGE_MODULE_EXPORT auto vk_on_unload(surge::window::window_t w,
                                      surge::renderer::vk::context ctx) -> int;

SURGE_MODULE_EXPORT auto vk_draw(surge::window::window_t w,
                                 surge::renderer::vk::context ctx) -> int;

SURGE_MODULE_EXPORT auto vk_update(surge::window::window_t w, surge::renderer::vk::context ctx,
                                   double dt) -> int;

SURGE_MODULE_EXPORT void vk_keyboard_event(surge::window::window_t w, int key, int scancode,
                                           int action, int mods);

SURGE_MODULE_EXPORT void vk_mouse_button_event(surge::window::window_t w, int button, int action,
                                               int mods);

SURGE_MODULE_EXPORT void vk_mouse_scroll_event(surge::window::window_t w, double xoffset,
                                               double yoffset);
}

#endif // SURGE_CORE_MODULE_DEFAULT_HPP