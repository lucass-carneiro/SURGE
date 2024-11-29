#include "default.hpp"

extern "C" SURGE_MODULE_EXPORT auto on_load() -> int { return 0; }

extern "C" SURGE_MODULE_EXPORT auto on_unload() -> int { return 0; }

extern "C" SURGE_MODULE_EXPORT auto draw() -> int { return 0; }

extern "C" SURGE_MODULE_EXPORT auto update(double) -> int { return 0; }

extern "C" SURGE_MODULE_EXPORT void keyboard_event(int, int, int, int) {}

extern "C" SURGE_MODULE_EXPORT void mouse_button_event(int, int, int) {}

extern "C" SURGE_MODULE_EXPORT void mouse_scroll_event(double, double) {}