#include "default.hpp"

extern "C" SURGE_MODULE_EXPORT auto on_load() noexcept -> int { return 0; }

extern "C" SURGE_MODULE_EXPORT auto on_unload() noexcept -> int { return 0; }

extern "C" SURGE_MODULE_EXPORT auto draw() noexcept -> int { return 0; }

extern "C" SURGE_MODULE_EXPORT auto update(double) noexcept -> int { return 0; }

extern "C" SURGE_MODULE_EXPORT void keyboard_event(int, int, int, int) noexcept {}

extern "C" SURGE_MODULE_EXPORT void mouse_button_event(int, int, int) noexcept {}

extern "C" SURGE_MODULE_EXPORT void mouse_scroll_event(double, double) noexcept {}