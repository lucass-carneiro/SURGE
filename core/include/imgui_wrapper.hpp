#ifndef SURGE_CORE_IMGUI_HPP
#define SURGE_CORE_IMGUI_HPP

#include "gl_atoms/sprite.hpp"
#include "gl_atoms/texture.hpp"
#include "imgui.h"
#include "options.hpp"

namespace surge::imgui {

enum class themes { light, dark };

struct create_config {
  bool keyboard_nav{false};
  bool gamepad_nav{false};
  themes theme{themes::dark};
};

namespace gl {

void create(create_config &&cfg) noexcept;
void destroy() noexcept;

void frame_begin() noexcept;
void frame_end() noexcept;

#ifdef SURGE_BUILD_TYPE_Debug
void texture_database_window(bool *open, const gl_atom::texture::database &tdb) noexcept;
void sprite_database_window(bool *open, const gl_atom::sprite::database &sdb) noexcept;
#endif

} // namespace gl

auto begin(const char *name, bool *open) noexcept -> bool;
void end() noexcept;

auto begin_main_menu_bar() noexcept -> bool;
void end_main_menu_bar() noexcept;

auto begin_menu(const char *name) noexcept -> bool;
auto menu_item(const char *name) noexcept -> bool;
void end_menu() noexcept;

auto begin_table(const char *name, int cols) noexcept -> bool;
void end_table() noexcept;

void table_setup_column(const char *name) noexcept;
void table_headers_row() noexcept;
void table_next_row() noexcept;
auto table_next_column() noexcept -> bool;

#if defined(SURGE_COMPILER_Clang) || defined(SURGE_COMPILER_GCC)
void text(const char *fmt, ...) noexcept __attribute__((format(printf, 1, 2)));
#else
void text(const char *fmt, ...) noexcept;
#endif

auto colapsing_header(const char *name) noexcept -> bool;

void register_mouse_callback(int button, int action, int mods) noexcept;
void register_mouse_scroll_callback(double xoffset, double yoffset) noexcept;

} // namespace surge::imgui

#endif // SURGE_CORE_IMGUI_HPP