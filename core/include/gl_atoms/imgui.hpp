#ifndef SURGE_CORE_GL_ATOM_IMGUI_HPP
#define SURGE_CORE_GL_ATOM_IMGUI_HPP

#include "texture.hpp"
#include "sprite.hpp"

namespace surge::gl_atom::imgui {

enum class themes { light, dark };

struct create_config {
  bool keyboard_nav{false};
  bool gamepad_nav{false};
  themes theme{themes::dark};
};

void create(create_config &&cfg) noexcept;
void destroy() noexcept;

void frame_begin() noexcept;
void frame_end() noexcept;

auto begin(const char *name, bool *open) noexcept -> bool;
void end() noexcept;

auto begin_main_menu_bar() noexcept -> bool;
void end_main_menu_bar() noexcept;

auto begin_menu(const char *name) noexcept -> bool;
auto menu_item(const char *name) noexcept -> bool;
void end_menu() noexcept;

auto begin_table(const char *name, int cols) noexcept -> bool;
void end_table() noexcept;

void register_mouse_callback(int button, int action, int mods) noexcept;
void register_mouse_scroll_callback(double xoffset, double yoffset) noexcept;

void texture_database_window(bool *open, const texture::database &tdb) noexcept;
void sprite_database_window(bool *open, const sprite::database &sdb) noexcept;

} // namespace surge::gl_atom::imgui

#endif // SURGE_CORE_GL_ATOM_IMGUI_HPP