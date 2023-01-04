#ifndef SURGE_LUA_WRAPPERS_HPP
#define SURGE_LUA_WRAPPERS_HPP

#include <filesystem>
#include <glm/mat4x4.hpp>
#include <lua.hpp>
#include <optional>

namespace surge {

auto lua_load_sprite(lua_State *L) noexcept -> int;
auto lua_drop_sprite(lua_State *L) noexcept -> int;
auto lua_draw_sprite(lua_State *L) noexcept -> int;
auto lua_scale_sprite(lua_State *L) noexcept -> int;
auto lua_move_sprite(lua_State *L) noexcept -> int;
auto lua_sheet_set(lua_State *L) noexcept -> int;
auto lua_sheet_next(lua_State *L) noexcept -> int;

auto lua_send_task_to(lua_State *L) noexcept -> int;
auto lua_run_task_at(lua_State *L) noexcept -> int;

auto lua_pre_loop_callback(lua_State *L) noexcept -> bool;
void lua_draw_callback(lua_State *L) noexcept;
void lua_update_callback(lua_State *L) noexcept;

auto lua_get_cursor_pos(lua_State *L) noexcept -> int;

} // namespace surge

#endif // SURGE_LUA_WRAPPERS_HPP