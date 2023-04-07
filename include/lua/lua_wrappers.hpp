#ifndef SURGE_LUA_WRAPPERS_HPP
#define SURGE_LUA_WRAPPERS_HPP

#include <filesystem>
#include <glm/mat4x4.hpp>
#include <luajit/lua.hpp>
#include <optional>

namespace surge {

auto lua_new_sprite(lua_State *L) noexcept -> int;
auto lua_drop_sprite(lua_State *L) noexcept -> int;
auto lua_draw_sprite(lua_State *L) noexcept -> int;
auto lua_scale_sprite(lua_State *L) noexcept -> int;
auto lua_move_sprite(lua_State *L) noexcept -> int;
auto lua_set_sprite_geometry(lua_State *L) noexcept -> int;
auto lua_sheet_set_indices(lua_State *L) noexcept -> int;
auto lua_sheet_set_offsets(lua_State *L) noexcept -> int;
auto lua_sheet_set_dimentions(lua_State *L) noexcept -> int;

auto lua_new_actor(lua_State *L) noexcept -> int;
auto lua_drop_actor(lua_State *L) noexcept -> int;
auto lua_draw_actor(lua_State *L) noexcept -> int;
auto lua_move_actor(lua_State *L) noexcept -> int;
auto lua_scale_actor(lua_State *L) noexcept -> int;
auto lua_update_actor(lua_State *L) noexcept -> int;
auto lua_change_actor_anim(lua_State *L) noexcept -> int;
auto lua_get_actor_anchor_coords(lua_State *L) noexcept -> int;
auto lua_actor_toggle_h_flip(lua_State *L) noexcept -> int;
auto lua_actor_toggle_v_flip(lua_State *L) noexcept -> int;

auto lua_send_task_to(lua_State *L) noexcept -> int;
auto lua_run_task_at(lua_State *L) noexcept -> int;

auto lua_pre_loop_callback(lua_State *L) noexcept -> bool;
void lua_draw_callback(lua_State *L) noexcept;
void lua_update_callback(lua_State *L) noexcept;

auto lua_get_cursor_pos(lua_State *L) noexcept -> int;
auto lua_get_key_state(lua_State *L) noexcept -> int;

auto lua_compute_heading(lua_State *L) noexcept -> int;

} // namespace surge

#endif // SURGE_LUA_WRAPPERS_HPP