#include "default.hpp"

extern "C" SURGE_MODULE_EXPORT auto on_load(surge::window::window_t) -> int { return 0; }

extern "C" SURGE_MODULE_EXPORT auto on_unload(surge::window::window_t) -> int { return 0; }

extern "C" SURGE_MODULE_EXPORT auto draw(surge::window::window_t) -> int { return 0; }

extern "C" SURGE_MODULE_EXPORT auto update(surge::window::window_t, double) -> int { return 0; }

extern "C" SURGE_MODULE_EXPORT void keyboard_event(surge::window::window_t, int, int, int, int) {}

extern "C" SURGE_MODULE_EXPORT void mouse_button_event(surge::window::window_t, int, int, int) {}

extern "C" SURGE_MODULE_EXPORT void mouse_scroll_event(surge::window::window_t, double, double) {}
