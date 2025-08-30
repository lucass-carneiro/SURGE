#ifndef SURGE_CORE_MODULE_SPRITES_HPP
#define SURGE_CORE_MODULE_SPRITES_HPP

#include "sc_module_context.hpp"
#include "sc_options.hpp"

#if defined(SURGE_COMPILER_GNU) && COMPILING_SURGE_MODULE_DEFAULT
#  define SURGE_MODULE_EXPORT __attribute__((__visibility__("default")))
#elif defined(SURGE_COMPILER_MSVC) && COMPILING_SURGE_MODULE_SPRITES
#  define SURGE_MODULE_EXPORT __declspec(dllexport)
#elif defined(SURGE_COMPILER_MSVC)
#  define SURGE_MODULE_EXPORT __declspec(dllimport)
#else
#  define SURGE_MODULE_EXPORT
#endif

extern "C" {

SURGE_MODULE_EXPORT auto on_load(surge::module::Context ctx) noexcept -> int;

SURGE_MODULE_EXPORT auto on_unload(surge::module::Context ctx) noexcept -> int;

SURGE_MODULE_EXPORT auto draw(surge::module::Context ctx) noexcept -> int;

SURGE_MODULE_EXPORT auto update(surge::module::Context ctx, double dt) noexcept -> int;

SURGE_MODULE_EXPORT void keyboard_event(surge::window::Window wnd, int key, int scancode,
                                        int action, int mods) noexcept;

SURGE_MODULE_EXPORT void mouse_button_event(surge::window::Window wnd, int button, int action,
                                            int mods) noexcept;

SURGE_MODULE_EXPORT void mouse_scroll_event(surge::window::Window wnd, double xoffset,
                                            double yoffset) noexcept;
}

#endif // SURGE_CORE_MODULE_SPRITES_HPP