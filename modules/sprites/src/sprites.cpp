#include "sprites.hpp"

extern "C" SURGE_MODULE_EXPORT auto on_load(surge::module::Context) noexcept -> int { return 0; }

extern "C" SURGE_MODULE_EXPORT auto on_unload(surge::module::Context) noexcept -> int { return 0; }

extern "C" SURGE_MODULE_EXPORT auto draw(surge::module::Context) noexcept -> int { return 0; }

extern "C" SURGE_MODULE_EXPORT auto update(surge::module::Context, double) noexcept -> int {
  return 0;
}

extern "C" SURGE_MODULE_EXPORT void keyboard_event(surge::window::Window, int, int, int,
                                                   int) noexcept {}

extern "C" SURGE_MODULE_EXPORT void mouse_button_event(surge::window::Window, int, int,
                                                       int) noexcept {}

extern "C" SURGE_MODULE_EXPORT void mouse_scroll_event(surge::window::Window, double,
                                                       double) noexcept {}
