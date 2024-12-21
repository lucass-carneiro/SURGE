#include "default.hpp"

extern "C" SURGE_MODULE_EXPORT auto gl_on_load(surge::window::window_t) -> int { return 0; }

extern "C" SURGE_MODULE_EXPORT auto gl_on_unload(surge::window::window_t) -> int { return 0; }

extern "C" SURGE_MODULE_EXPORT auto gl_draw(surge::window::window_t) -> int { return 0; }

extern "C" SURGE_MODULE_EXPORT auto gl_update(surge::window::window_t, double) -> int { return 0; }

extern "C" SURGE_MODULE_EXPORT void gl_keyboard_event(surge::window::window_t, int, int, int, int) {
}

extern "C" SURGE_MODULE_EXPORT void gl_mouse_button_event(surge::window::window_t, int, int, int) {}

extern "C" SURGE_MODULE_EXPORT void gl_mouse_scroll_event(surge::window::window_t, double, double) {
}

extern "C" SURGE_MODULE_EXPORT auto vk_on_load(surge::window::window_t,
                                               surge::renderer::vk::context) -> int {
  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto vk_on_unload(surge::window::window_t,
                                                 surge::renderer::vk::context) -> int {
  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto vk_draw(surge::window::window_t,
                                            surge::renderer::vk::context) -> int {
  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto vk_update(surge::window::window_t, surge::renderer::vk::context,
                                              double) -> int {
  return 0;
}

extern "C" SURGE_MODULE_EXPORT void vk_keyboard_event(surge::window::window_t, int, int, int, int) {
}

extern "C" SURGE_MODULE_EXPORT void vk_mouse_button_event(surge::window::window_t, int, int, int) {}

extern "C" SURGE_MODULE_EXPORT void vk_mouse_scroll_event(surge::window::window_t, double, double) {
}
