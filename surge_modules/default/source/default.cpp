#include "default.hpp"

#include "logging.hpp"

extern "C" {

SURGE_MODULE_EXPORT void on_load() noexcept {
  log_info("This is called when the module gets loaded");
}

SURGE_MODULE_EXPORT void on_unload() noexcept {
  log_info("This is called when the module gets unloaded");
}

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