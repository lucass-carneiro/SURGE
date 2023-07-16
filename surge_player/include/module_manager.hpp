#ifndef SURGE_MODULE_MANAGER_HPP
#define SURGE_MODULE_MANAGER_HPP

#include <optional>

namespace surge::modules {

using load_unload_callback_t = void (*)();
using update_callback_t = void (*)(double);
using buton_key_callback_t = void (*)(int, int, int);
using scroll_callback_t = void (*)(double, double);

struct module_t {
  void *lib_handle;

  load_unload_callback_t on_load;
  load_unload_callback_t on_unload;

  load_unload_callback_t draw;
  update_callback_t update;

  buton_key_callback_t keyboard_event;
  buton_key_callback_t mouse_button_event;
  scroll_callback_t mouse_scroll_event;
};

auto load(const char *module_name) noexcept -> std::optional<module_t>;
void unload(module_t &module) noexcept;

} // namespace surge::modules

#endif // SURGE_MODULE_MANAGER_HPP