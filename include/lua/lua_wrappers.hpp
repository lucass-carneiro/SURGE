#ifndef SURGE_LUA_WRAPPERS_HPP
#define SURGE_LUA_WRAPPERS_HPP

// clang-format off
#include <lua.hpp>
// clang-format on

namespace surge {

auto lua_load_image(lua_State *L) noexcept -> int;
auto lua_drop_image(lua_State *L) noexcept -> int;

} // namespace surge

#endif // SURGE_LUA_WRAPPERS_HPP