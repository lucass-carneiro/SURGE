#ifndef SURGE_CORE_GL_ATOM_IMGUI_HPP
#define SURGE_CORE_GL_ATOM_IMGUI_HPP

#include "sc_imgui.hpp"
#include "sc_options.hpp"
#include "sprite_database.hpp"
#include "texture.hpp"

namespace surge::gl_atom::imgui {

auto create(surge::imgui::create_config &&cfg = surge::imgui::create_config{}) -> ImGuiContext *;
void destroy(ImGuiContext *ctx);

void frame_begin();
void frame_end();

#ifdef SURGE_BUILD_TYPE_Debug
void texture_database_window(bool *open, const gl_atom::texture::database &tdb);
void sprite_database_window(bool *open, const gl_atom::sprite_database::database sdb);
#endif

} // namespace surge::gl_atom::imgui

#endif // SURGE_CORE_GL_ATOM_IMGUI_HPP