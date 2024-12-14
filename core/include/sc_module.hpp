#ifndef SURGE_CORE_MODULE_HPP
#define SURGE_CORE_MODULE_HPP

#include "sc_container_types.hpp"
#include "sc_error_types.hpp"
#include "sc_glfw_includes.hpp"
#include "sc_options.hpp"

// clang-format off
#ifdef SURGE_SYSTEM_Windows
#  include <windows.h>
#  include <libloaderapi.h>
#else
#  include <dlfcn.h>
#endif
// clang-format on

#include <tl/expected.hpp>

namespace surge::module {

#ifdef SURGE_SYSTEM_Windows
using handle_t = HMODULE;
#else
using handle_t = void *;
#endif

struct gl_api {
  using on_load_t = int (*)(surge::window::window_t);
  using on_unload_t = int (*)(surge::window::window_t);
  using draw_t = int (*)(surge::window::window_t);
  using update_t = int (*)(surge::window::window_t, double);

  using keyboard_event_t = void (*)(surge::window::window_t, int, int, int, int);
  using mouse_button_event_t = void (*)(surge::window::window_t, int, int, int);
  using mouse_scroll_event_t = void (*)(surge::window::window_t, double, double);

  on_load_t on_load;
  on_unload_t on_unload;

  draw_t draw;
  update_t update;

  keyboard_event_t keyboard_event;
  mouse_button_event_t mouse_button_event;
  mouse_scroll_event_t mouse_scroll_event;
};

struct vk_api {
  using on_load_t = int (*)(surge::window::window_t);
  using on_unload_t = int (*)(surge::window::window_t);
  using draw_t = int (*)(surge::window::window_t);
  using update_t = int (*)(surge::window::window_t, double);

  using keyboard_event_t = void (*)(surge::window::window_t, int, int, int, int);
  using mouse_button_event_t = void (*)(surge::window::window_t, int, int, int);
  using mouse_scroll_event_t = void (*)(surge::window::window_t, double, double);

  on_load_t on_load;
  on_unload_t on_unload;

  draw_t draw;
  update_t update;

  keyboard_event_t keyboard_event;
  mouse_button_event_t mouse_button_event;
  mouse_scroll_event_t mouse_scroll_event;
};

auto get_name(handle_t module, usize max_size = 256) noexcept -> tl::expected<string, error>;

auto load(const char *path) noexcept -> tl::expected<handle_t, error>;
void unload(handle_t module) noexcept;
auto reload(handle_t module) noexcept -> tl::expected<handle_t, error>;

auto get_gl_api(handle_t module) noexcept -> tl::expected<gl_api, error>;
auto get_vk_api(handle_t module) noexcept -> tl::expected<vk_api, error>;

auto set_module_path() noexcept -> bool;

} // namespace surge::module

#endif // SURGE_CORE_MODULE_HPP