#ifndef SURGE_LUA_WRAPPERS_HPP
#define SURGE_LUA_WRAPPERS_HPP

// clang-format off
#include <lua.hpp>
// clang-format on

#include <optional>

namespace surge {

auto lua_load_image(lua_State *L) noexcept -> int;
auto lua_drop_image(lua_State *L) noexcept -> int;

auto lua_create_program(lua_State *L) noexcept -> int;
auto lua_get_shader_program_idx(lua_State *L) noexcept -> std::optional<lua_Integer>;

auto lua_load_callback(lua_State *L) noexcept -> bool;

} // namespace surge

#endif // SURGE_LUA_WRAPPERS_HPP