#include "geometry_utils.hpp"
#include "logging_system/logging_system.hpp"
#include "lua/lua_wrappers.hpp"

auto surge::lua_compute_heading(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 3) {
    log_warn("Function compute_heading expected 3 arguments and instead got "
             "{} arguments. Returning nil",
             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isnumber(L, 2)) {
    log_warn("Function compute_heading expected argument 1 to be a float. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isnumber(L, 2)) {
    log_warn("Function compute_heading expected argument 2 to be a float. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isnumber(L, 3)) {
    log_warn("Function compute_heading expected argument 3 to be a float. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  // Data recovery
  const auto a{static_cast<float>(lua_tonumber(L, 1))};
  const auto b{static_cast<float>(lua_tonumber(L, 2))};
  const auto c{static_cast<float>(lua_tonumber(L, 3))};

  // Internal call
  const auto heading{compute_heading_to(glm::vec3{a, b, c})};
  lua_pushinteger(L, static_cast<lua_Integer>(heading));

  return 1;
}