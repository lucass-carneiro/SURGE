#include "allocator.hpp"
#include "entities/background.hpp"
#include "log.hpp"
#include "lua/lua_wrappers.hpp"

auto surge::lua_new_background(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 1) {
    log_warn("Function new expected 1 arguments and instead got {} "
             "arguments. Returning nil",
             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isstring(L, 1)) {
    log_warn("Fucntion new expects arguents 1 to be strings. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  // Argument extraction
  const char *sheet_path_str{lua_tostring(L, 1)};

  // Internal call
  auto background_buffer{mi_malloc(sizeof(background))};
  background *background_ptr{new (background_buffer) background(sheet_path_str)};

  // Pass this pointer to the Lua VM as userdata
  auto vm_background_ptr{static_cast<background **>(lua_newuserdata(L, sizeof(void *)))};
  *vm_background_ptr = background_ptr;

  lua_getglobal(L, "surge");
  lua_getfield(L, -1, "background");
  lua_getfield(L, -1, "background_meta");
  lua_setmetatable(L, -4);
  lua_pop(L, 2);

  return 1;
}

auto surge::lua_drop_background(lua_State *L) noexcept -> int {
  // Data recovery
  auto vm_background_ptr{static_cast<background **>(lua_touserdata(L, 1))};
  auto background_ptr{*vm_background_ptr};

  // Data cleanup
  mi_free(background_ptr);

  return 0;
}

[[nodiscard]] static auto is_background(lua_State *L, const char *func_name) noexcept -> bool {
  if (!lua_isuserdata(L, 1)) {
    surge::log_warn("Function {} expected 1 user data argument. Returning nil", func_name);
    return false;
  }

  if (!lua_getmetatable(L, 1)) {
    surge::log_warn("User data does not have a metatable");
    return false;
  }

  lua_getfield(L, -1, "__name");
  const auto name{lua_tostring(L, -1)};

  if (std::strcmp(name, "surge::background") != 0) {
    surge::log_warn("Expected surge::background userdata and recieved {}", name);
    return false;
  }

  lua_pop(L, 2);
  return true;
}

auto surge::lua_draw_background(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 1) {
    log_warn("Function draw expected 1 user data arguments and instead got "
             "{} arguments. Returning nil",
             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!is_background(L, "draw")) {
    lua_pushnil(L);
    return 1;
  }

  // Data recovery
  auto vm_background_ptr{static_cast<background **>(lua_touserdata(L, 1))};
  auto background_ptr{*vm_background_ptr};

  // Internal call
  background_ptr->draw();

  lua_pop(L, 1);

  return 0;
}