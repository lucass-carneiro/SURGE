#ifndef SURGE_CORE_MODULE_HPP
#define SURGE_CORE_MODULE_HPP

#include "sc_container_types.hpp"
#include "sc_error_types.hpp"
#include "sc_module_context.hpp"
#include "sc_options.hpp"
#include "sc_window.hpp"

// clang-format off
#ifdef SURGE_SYSTEM_Windows
#  include <windows.h>
#  include <libloaderapi.h>
#else
#  include <dlfcn.h>
#endif
// clang-format on

#include <optional>
#include <tl/expected.hpp>

namespace surge::module {

#ifdef SURGE_SYSTEM_Windows
using Handle = HMODULE;
#else
using Handle = void *;
#endif

struct Api {
  using on_load_t = int (*)(Context) noexcept;
  using on_unload_t = int (*)(Context) noexcept;
  using draw_t = int (*)(Context) noexcept;
  using update_t = int (*)(Context, double) noexcept;

  using keyboard_event_t = void (*)(window::Window, int, int, int, int) noexcept;
  using mouse_button_event_t = void (*)(window::Window, int, int, int) noexcept;
  using mouse_scroll_event_t = void (*)(window::Window, double, double) noexcept;

  on_load_t on_load;
  on_unload_t on_unload;

  draw_t draw;
  update_t update;

  keyboard_event_t keyboard_event;
  mouse_button_event_t mouse_button_event;
  mouse_scroll_event_t mouse_scroll_event;
};

auto get_name(Handle module, usize max_size = 256) noexcept -> Result<containers::mimalloc::String>;

#ifdef SURGE_SYSTEM_Windows
auto get_func_addr(Handle module, const char *func_name) -> std::optional<FARPROC>;
#else
auto get_func_addr(Handle module, const char *func_name) -> std::optional<void *>;
#endif

auto load(const char *path) noexcept -> Result<Handle>;
void unload(Handle module) noexcept;
auto reload(Handle module) noexcept -> Result<Handle>;

auto get_api(Handle module) noexcept -> Result<Api>;

auto set_module_path() noexcept -> bool;

void bind_input_callbacks(Context ctx, Handle hdl, const Api &api);
void unbind_input_callbacks(Context ctx);

} // namespace surge::module

#endif // SURGE_CORE_MODULE_HPP