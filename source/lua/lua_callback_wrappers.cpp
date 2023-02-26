#include "log.hpp"
#include "lua/lua_wrappers.hpp"
#include "window.hpp"

auto surge::lua_pre_loop_callback(lua_State *L) noexcept -> bool {
  lua_getglobal(L, "surge");
  lua_getfield(L, -1, "pre_loop");

  const auto pcall_result{lua_pcall(L, 0, 0, 0)};

  if (pcall_result != 0) {
    glog<log_event::error>("Unable to call surge.pre_loop: {}", lua_tostring(L, -1));
    lua_pop(L, 2);
    return false;
  } else {
    lua_pop(L, 1);
    return true;
  }
}

void surge::lua_draw_callback(lua_State *L) noexcept {
  lua_getglobal(L, "surge");
  lua_getfield(L, -1, "draw");

  const auto pcall_result{lua_pcall(L, 0, 0, 0)};

  if (pcall_result != 0) {
    glog<log_event::error>("Unable to call surge.draw: {}", lua_tostring(L, -1));
    lua_pop(L, 2);
  } else {
    lua_pop(L, 1);
  }
}

void surge::lua_update_callback(lua_State *L) noexcept {
  lua_getglobal(L, "surge");
  lua_getfield(L, -1, "update");
  lua_pushnumber(L, global_engine_window::get().get_frame_dt());

  const auto pcall_result{lua_pcall(L, 1, 0, 0)};

  if (pcall_result != 0) {
    glog<log_event::error>("Unable to call surge.update: {}", lua_tostring(L, -1));
    lua_pop(L, 3);
  } else {
    lua_pop(L, 1);
  }
}

void surge::glfw_key_callback(GLFWwindow *, int key, int, int action, int mods) noexcept {

  // VM defined actions
  lua_State *L{global_lua_states::get().at(0).get()};

  lua_getglobal(L, "surge");
  lua_getfield(L, -1, "key_event");
  lua_pushinteger(L, key);
  lua_pushinteger(L, action);
  lua_pushinteger(L, mods);

  const auto pcall_result{lua_pcall(L, 3, 0, 0)};

  if (pcall_result != 0) {
    glog<log_event::error>("Unable to call surge.key_event: {}", lua_tostring(L, -1));
    lua_pop(L, 5);
  } else {
    lua_pop(L, 1);
  }
}

auto surge::lua_get_cursor_pos(lua_State *L) noexcept -> int {
  auto [x, y] = global_engine_window::get().get_cursor_pos();
  lua_pushnumber(L, x);
  lua_pushnumber(L, y);
  lua_pushnumber(L, 0.0);
  return 3;
}

void surge::glfw_mouse_button_callback(GLFWwindow *, int button, int action, int mods) noexcept {
  // Recover main VM state
  lua_State *L{global_lua_states::get().at(0).get()};

  lua_getglobal(L, "surge");
  lua_getfield(L, -1, "mouse_button_event");
  lua_pushinteger(L, button);
  lua_pushinteger(L, action);
  lua_pushinteger(L, mods);

  const auto pcall_result{lua_pcall(L, 3, 0, 0)};

  if (pcall_result != 0) {
    glog<log_event::error>("Unable to call surge.mouse_button_event: {}", lua_tostring(L, -1));
    lua_pop(L, 5);
  } else {
    lua_pop(L, 1);
  }
}

void surge::glfw_scroll_callback(GLFWwindow *, double xoffset, double yoffset) noexcept {
  // Recover main VM state
  lua_State *L{global_lua_states::get().at(0).get()};

  lua_getglobal(L, "surge");
  lua_getfield(L, -1, "mouse_scroll_event");
  lua_pushnumber(L, xoffset);
  lua_pushnumber(L, yoffset);

  const auto pcall_result{lua_pcall(L, 2, 0, 0)};

  if (pcall_result != 0) {
    glog<log_event::error>("Unable to call surge.mouse_scroll_event: {}", lua_tostring(L, -1));
    lua_pop(L, 5);
  } else {
    lua_pop(L, 1);
  }
}