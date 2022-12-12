#include "lua/lua_wrappers.hpp"

#include "log.hpp"
#include "lua/lua_bindings.hpp"
#include "mesh/sprite.hpp"
#include "opengl/create_program.hpp"
#include "thread_allocators.hpp"
#include "window.hpp"

auto surge::lua_load_sprite(lua_State *L) noexcept -> int {
  glog<log_event::message>("lua_load_sprite called");

  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 4) {
    glog<log_event::warning>("Function load_sprite expected 4 arguments and instead got {} "
                             "arguments. Returning nil",
                             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!(lua_isstring(L, 1) || lua_isstring(L, 2) || lua_isnumber(L, 3) || lua_isnumber(L, 4))) {
    glog<log_event::warning>(
        "Fucntion load_image expects 2 string arguments and 2 integer arguments. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  // Argument extraction
  const char *path_str{lua_tostring(L, 1)};
  const char *ext_str{lua_tostring(L, 2)};
  const lua_Integer sheet_width{lua_tointeger(L, 3)};
  const lua_Integer sheet_height{lua_tointeger(L, 4)};

  // VM index recovery
  lua_getglobal(L, "surge");
  lua_getfield(L, -1, "vm_index");
  const auto vm_index{static_cast<std::size_t>(lua_tointeger(L, -1))};
  lua_pop(L, 2);

  // Internal call
  auto sprite_buffer{global_thread_allocators::get().at(vm_index).get()->malloc(sizeof(sprite))};
  sprite *sprite_ptr{new (sprite_buffer) sprite(global_thread_allocators::get().at(vm_index).get(),
                                                path_str, ext_str, sheet_width, sheet_height,
                                                buffer_usage_hint::static_draw)};

  // Pass this pointer to the Lua VM as userdata
  auto vm_sprite_ptr{static_cast<sprite **>(lua_newuserdata(L, sizeof(void *)))};
  *vm_sprite_ptr = sprite_ptr;

  lua_getglobal(L, "surge");
  lua_getfield(L, -1, "sprite_meta");
  lua_setmetatable(L, -3);
  lua_pop(L, 1);

  return 1;
}

auto surge::lua_drop_sprite(lua_State *L) noexcept -> int {
  glog<log_event::message>("lua_drop_sprite called");

  // Data recovery
  auto vm_sprite_ptr{static_cast<sprite **>(lua_touserdata(L, 1))};
  auto sprite_ptr{*vm_sprite_ptr};

  // VM index recovery
  lua_getglobal(L, "surge");
  lua_getfield(L, -1, "vm_index");
  const auto vm_index{static_cast<std::size_t>(lua_tointeger(L, -1))};
  lua_pop(L, 2);

  // Data cleanup
  global_thread_allocators::get().at(vm_index)->free(sprite_ptr);

  return 0;
}

[[nodiscard]] auto is_sprite(lua_State *L, const char *func_name) noexcept -> bool {
  if (!lua_isuserdata(L, 1)) {
    surge::glog<surge::log_event::warning>(
        "Function {} expected 1 user data argument. Returning nil", func_name);
    return false;
  }

  if (!lua_getmetatable(L, 1)) {
    surge::glog<surge::log_event::warning>("User data does not have a metatable");
    return false;
  }

  lua_getfield(L, -1, "__name");
  const auto name{lua_tostring(L, -1)};

  if (std::strcmp(name, "surge::sprite") != 0) {
    surge::glog<surge::log_event::warning>("Expected surge::sprite userdata and recieved {}", name);
    return false;
  }

  lua_pop(L, 2);
  return true;
}

auto surge::lua_draw_sprite(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 1) {
    glog<log_event::warning>("Function draw_sprite expected 1 user data arguments and instead got "
                             "{} arguments. Returning nil",
                             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!is_sprite(L, "draw_sprite")) {
    lua_pushnil(L);
    return 1;
  }

  // Data recovery
  auto vm_sprite_ptr{static_cast<sprite **>(lua_touserdata(L, 1))};
  auto sprite_ptr{*vm_sprite_ptr};

  // Internal call
  sprite_ptr->draw(global_engine_window::get().get_shader_program());

  lua_pop(L, 1);

  return 0;
}

auto surge::lua_scale_sprite(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 4) {
    glog<log_event::warning>("Function scale_sprite expected 4 arguments and instead got "
                             "{} arguments. Returning nil",
                             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!is_sprite(L, "scale_sprite")) {
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isnumber(L, 2)) {
    glog<log_event::warning>(
        "Function scale_sprite expected second argument to be a number. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isnumber(L, 3)) {
    glog<log_event::warning>(
        "Function scale_sprite expected third argument to be a number. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isnumber(L, 4)) {
    glog<log_event::warning>(
        "Function scale_sprite expected fourth argument to be a number. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  // Data recovery
  auto vm_sprite_ptr{static_cast<sprite **>(lua_touserdata(L, 1))};
  auto sprite_ptr{*vm_sprite_ptr};
  const auto sx{static_cast<float>(lua_tonumber(L, 2))}, sy{static_cast<float>(lua_tonumber(L, 3))},
      sz{static_cast<float>(lua_tonumber(L, 4))};

  // Internal call
  sprite_ptr->scale(global_engine_window::get().get_shader_program(), glm::vec3{sx, sy, sz});

  lua_pop(L, 1);

  return 0;
}

auto surge::lua_move_sprite(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 4) {
    glog<log_event::warning>("Function move_sprite expected 4 arguments and instead got "
                             "{} arguments. Returning nil",
                             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!is_sprite(L, "move_sprite")) {
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isnumber(L, 2)) {
    glog<log_event::warning>(
        "Function move_sprite expected second argument to be a number. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isnumber(L, 3)) {
    glog<log_event::warning>(
        "Function move_sprite expected third argument to be a number. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isnumber(L, 4)) {
    glog<log_event::warning>(
        "Function move_sprite expected fourth argument to be a number. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  // Data recovery
  auto vm_sprite_ptr{static_cast<sprite **>(lua_touserdata(L, 1))};
  auto sprite_ptr{*vm_sprite_ptr};
  const auto mx{static_cast<float>(lua_tonumber(L, 2))}, my{static_cast<float>(lua_tonumber(L, 3))},
      mz{static_cast<float>(lua_tonumber(L, 4))};

  // Internal call
  sprite_ptr->move(global_engine_window::get().get_shader_program(), glm::vec3{mx, my, mz});

  lua_pop(L, 1);

  return 0;
}

auto surge::lua_sheet_set(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 3) {
    glog<log_event::warning>("Function sheet_set expected 3 arguments and instead got "
                             "{} arguments. Returning nil",
                             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!is_sprite(L, "sheet_set")) {
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isnumber(L, 2)) {
    glog<log_event::warning>(
        "Function sheet_set expected second argument to be a number. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isnumber(L, 3)) {
    glog<log_event::warning>(
        "Function sheet_set expected third argument to be a number. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  // Data recovery
  auto vm_sprite_ptr{static_cast<sprite **>(lua_touserdata(L, 1))};
  auto sprite_ptr{*vm_sprite_ptr};
  const auto i{static_cast<GLint>(lua_tonumber(L, 2))}, j{static_cast<GLint>(lua_tonumber(L, 3))};

  // Internal call
  sprite_ptr->sheet_set(glm::ivec2{i, j});

  lua_pop(L, 1);

  return 0;
}

auto surge::lua_sheet_next(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 1) {
    glog<log_event::warning>("Function sheet_next expected 1 argument and instead got "
                             "{} arguments. Returning nil",
                             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!is_sprite(L, "sheet_next")) {
    lua_pushnil(L);
    return 1;
  }

  // Data recovery
  auto vm_sprite_ptr{static_cast<sprite **>(lua_touserdata(L, 1))};
  auto sprite_ptr{*vm_sprite_ptr};

  // Internal call
  sprite_ptr->sheet_next();

  lua_pop(L, 1);

  return 0;
}

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