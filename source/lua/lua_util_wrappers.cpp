#include "log.hpp"
#include "lua/lua_states.hpp"
#include "lua/lua_utils.hpp"
#include "lua/lua_wrappers.hpp"

auto surge::lua_prealloc_table(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 1) {
    log_warn("Function prealloc_table expects 1 integer argument. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isnumber(L, 1)) {
    log_warn("Function prealloc_table expects it's argument to be a number. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  // Create table to prealloc
  const auto size{static_cast<int>(lua_tointeger(L, 1))};
  lua_createtable(L, size, 0);

  return 1;
}