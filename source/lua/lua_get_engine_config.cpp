#include "lua/lua_utils.hpp"

auto surge::lua_get_engine_config(lua_State *L) noexcept -> std::optional<lua_engine_config> {
  lua_engine_config config{};

  const auto stack_top{lua_gettop(L)};

  lua_getglobal(L, "surge");
  lua_getfield(L, -1, "config");

  lua_getfield(L, -1, "window_width");
  if (!lua_isnumber(L, -1)) {
    log_error("The value stored in the field window_width is not a number");
    lua_settop(L, stack_top);
    return {};
  } else {
    config.window_width = lua_tointeger(L, -1);
    lua_pop(L, 1);
  }

  lua_getfield(L, -1, "window_height");
  if (!lua_isnumber(L, -1)) {
    log_error("The value stored in the field window_height is not a number");
    lua_settop(L, stack_top);
    return {};
  } else {
    config.window_height = lua_tointeger(L, -1);
    lua_pop(L, 1);
  }

  lua_getfield(L, -1, "window_name");
  if (!lua_isstring(L, -1)) {
    log_error("The value stored in the field window_name is not a string");
    lua_settop(L, stack_top);
    return {};
  } else {
    config.window_name = lua_tostring(L, -1);
    lua_pop(L, 1);
  }

  lua_getfield(L, -1, "windowed");
  if (!lua_isboolean(L, -1)) {
    log_error("The value stored in the field windowed is not a boolean");
    lua_settop(L, stack_top);
    return {};
  } else {
    config.windowed = static_cast<lua_Boolean>(lua_toboolean(L, -1));
    lua_pop(L, 1);
  }

  lua_getfield(L, -1, "window_monitor_index");
  if (!lua_isnumber(L, -1)) {
    log_error("The value stored in the field window_monitor_index is not a number");
    lua_settop(L, stack_top);
    return {};
  } else {
    config.window_monitor_index = lua_tointeger(L, -1);
    lua_pop(L, 1);
  }

  lua_getfield(L, -1, "show_cursor");
  if (!lua_isboolean(L, -1)) {
    log_error("The value stored in the field show_cursor is not a boolean");
    lua_settop(L, stack_top);
    return {};
  } else {
    config.show_cursor = static_cast<lua_Boolean>(lua_toboolean(L, -1));
    lua_pop(L, 1);
  }

  lua_getfield(L, -1, "show_debug_objects");
  if (!lua_isnumber(L, -1)) {
    log_error("The value stored in the field show_debug_objects is not a number");
    lua_settop(L, stack_top);
    return {};
  } else {
    config.show_debug_objects = lua_tointeger(L, -1);
    lua_pop(L, 1);
  }

  lua_getfield(L, -1, "engine_root_dir");
  if (!lua_isstring(L, -1)) {
    log_error("The value stored in the field engine_root_dir is not a string");
    lua_settop(L, stack_top);
    return {};
  } else {
    config.root_dir = lua_tostring(L, -1);
    lua_pop(L, 1);
  }

  lua_getfield(L, -1, "clear_color");
  if (!lua_istable(L, -1)) {
    log_error("The value stored in the field clear_color is not a table");
    lua_settop(L, stack_top);
    return {};

  } else {

    for (int i = 1; auto &color : config.clear_color) {
      lua_rawgeti(L, -1, i);

      if (lua_isnoneornil(L, -1) || !lua_isnumber(L, -1)) {
        log_error("The value stored in the field clear_color[{}] is nill, none or not a number", i);
        lua_settop(L, stack_top);
        return {};
      }

      color = lua_tonumber(L, -1);
      lua_pop(L, 1);
      i++;
    }

    lua_pop(L, 1);
  }

  lua_settop(L, stack_top);

  return config;
}