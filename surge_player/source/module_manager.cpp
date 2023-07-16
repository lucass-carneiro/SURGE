#include "module_manager.hpp"

#include "logging.hpp"
#include "options.hpp"

#ifdef SURGE_SYSTEM_IS_POSIX
#  include <dlfcn.h>
#else
#  error "Unable to load and unload modules in this OS
#endif

#ifdef SURGE_SYSTEM_IS_POSIX

auto surge::modules::load(const char *module_name) noexcept -> std::optional<module_t> {
  // Load and get handle
  auto handle{dlopen(module_name, RTLD_LAZY)};
  if (!handle) {
    log_error("Unable to load library %s", dlerror());
    return {};
  }

  // Find callbacks
  (void)dlerror();
  auto on_load_handle{dlsym(handle, "on_load")};
  if (!on_load_handle) {
    log_error("Unable to find symbol %s", dlerror());
    return {};
  }

  (void)dlerror();
  auto on_unload_handle{dlsym(handle, "on_unload")};
  if (!on_unload_handle) {
    log_error("Unable to find symbol %s", dlerror());
    return {};
  }

  (void)dlerror();
  auto draw_handle{dlsym(handle, "draw")};
  if (!draw_handle) {
    log_error("Unable to find symbol %s", dlerror());
    return {};
  }

  (void)dlerror();
  auto update_handle{dlsym(handle, "update")};
  if (!update_handle) {
    log_error("Unable to find symbol %s", dlerror());
    return {};
  }

  (void)dlerror();
  auto keyboard_event_handle{dlsym(handle, "keyboard_event")};
  if (!keyboard_event_handle) {
    log_error("Unable to find symbol %s", dlerror());
    return {};
  }

  (void)dlerror();
  auto mouse_button_event_handle{dlsym(handle, "mouse_button_event")};
  if (!mouse_button_event_handle) {
    log_error("Unable to find symbol %s", dlerror());
    return {};
  }

  (void)dlerror();
  auto mouse_scroll_event_handle{dlsym(handle, "mouse_scroll_event")};
  if (!mouse_scroll_event_handle) {
    log_error("Unable to find symbol %s", dlerror());
    return {};
  }

  // Construct module and call on_load()

  module_t module{handle,
                  reinterpret_cast<load_unload_callback_t>(on_load_handle),
                  reinterpret_cast<load_unload_callback_t>(on_unload_handle),
                  reinterpret_cast<load_unload_callback_t>(draw_handle),
                  reinterpret_cast<update_callback_t>(update_handle),
                  reinterpret_cast<buton_key_callback_t>(keyboard_event_handle),
                  reinterpret_cast<buton_key_callback_t>(mouse_button_event_handle),
                  reinterpret_cast<scroll_callback_t>(mouse_scroll_event_handle)};

  module.on_load();

  return module;
}

void surge::modules::unload(module_t &module) noexcept {
  module.on_unload();

  const auto result{dlclose(module.lib_handle)};
  if (result != 0) {
    log_error("Unable to close library %s", dlerror());
  }
}

#endif