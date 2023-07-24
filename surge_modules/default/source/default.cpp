#include "default.hpp"

#include "allocators.hpp"
#include "logging.hpp"

#include <EASTL/vector.h>

extern "C" {

SURGE_MODULE_EXPORT void on_load() noexcept { log_info("On load"); }

SURGE_MODULE_EXPORT void on_unload() noexcept { log_info("On unload"); }

SURGE_MODULE_EXPORT void draw() noexcept { log_info("Draw called"); }

SURGE_MODULE_EXPORT void update(double dt) noexcept {
  log_info("Update called with dt = %.16f", dt);
}

SURGE_MODULE_EXPORT void keyboard_event(int key, int action, int mods) noexcept {
  log_info("Keyboard event. Key: %i, Action: %i, Mods: %i", key, action, mods);
}

SURGE_MODULE_EXPORT void mouse_button_event(int button, int action, int mods) noexcept {
  log_info("Mouse button event. Button: %i, Action: %i, Mods: %i", button, action, mods);
}

SURGE_MODULE_EXPORT void mouse_scroll_event(double xoffset, double yoffset) noexcept {
  log_info("Mouse scroll event. X offset: %.16f, Y offset: %.16f", xoffset, yoffset);
}
}