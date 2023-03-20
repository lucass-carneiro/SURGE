#include "allocator.hpp"
#include "entities/sprite.hpp"
#include "lua/lua_wrappers.hpp"
#include "window.hpp"

auto surge::lua_new_sprite(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 2) {
    glog<log_event::warning>("Function load_sprite expected 2 arguments and instead got {} "
                             "arguments. Returning nil",
                             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!(lua_isstring(L, 1) || lua_isstring(L, 2))) {
    glog<log_event::warning>("Fucntion load_sprite expects 2 string arguments. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  // Argument extraction
  const char *path_str{lua_tostring(L, 1)};
  const char *ext_str{lua_tostring(L, 2)};

  // Internal call
  auto sprite_buffer{mi_malloc(sizeof(sprite))};
  sprite *sprite_ptr{new (sprite_buffer) sprite(path_str, ext_str, buffer_usage_hint::static_draw)};

  // Pass this pointer to the Lua VM as userdata
  auto vm_sprite_ptr{static_cast<sprite **>(lua_newuserdata(L, sizeof(void *)))};
  *vm_sprite_ptr = sprite_ptr;

  lua_getglobal(L, "surge");
  lua_getfield(L, -1, "sprite_meta");
  lua_setmetatable(L, -4);
  lua_pop(L, 2);

  return 1;
}

auto surge::lua_drop_sprite(lua_State *L) noexcept -> int {
  // Data recovery
  auto vm_sprite_ptr{static_cast<sprite **>(lua_touserdata(L, 1))};
  auto sprite_ptr{*vm_sprite_ptr};

  // Data cleanup
  mi_free(sprite_ptr);

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

  for (int i = 2; i <= 4; i++) {
    if (!lua_isnumber(L, i)) {
      glog<log_event::warning>(
          "Function scale_sprite expected argument {} to be a number. Returning nil", i);
      lua_pushnil(L);
      return 1;
    }
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

  for (int i = 2; i <= 4; i++) {
    if (!lua_isnumber(L, i)) {
      glog<log_event::warning>(
          "Function move_sprite expected argument {} to be a number. Returning nil", i);
      lua_pushnil(L);
      return 1;
    }
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

auto surge::lua_set_sprite_geometry(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 7) {
    glog<log_event::warning>("Function set_sprite_geometry expected 7 arguments and instead got "
                             "{} arguments. Returning nil",
                             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!is_sprite(L, "set_sprite_geometry")) {
    lua_pushnil(L);
    return 1;
  }

  for (int i = 2; i <= 7; i++) {
    if (!lua_isnumber(L, i)) {
      glog<log_event::warning>(
          "Function set_sprite_geometry expected argument {} to be a number. Returning nil", i);
      lua_pushnil(L);
      return 1;
    }
  }

  // Data recovery
  auto vm_sprite_ptr{static_cast<sprite **>(lua_touserdata(L, 1))};
  auto sprite_ptr{*vm_sprite_ptr};
  const auto mx{static_cast<float>(lua_tonumber(L, 2))}, my{static_cast<float>(lua_tonumber(L, 3))},
      mz{static_cast<float>(lua_tonumber(L, 4))}, sx{static_cast<float>(lua_tonumber(L, 5))},
      sy{static_cast<float>(lua_tonumber(L, 6))}, sz{static_cast<float>(lua_tonumber(L, 7))};

  // Internal call
  sprite_ptr->set_geometry(global_engine_window::get().get_shader_program(), glm::vec3{mx, my, mz},
                           glm::vec3{sx, sy, sz});

  lua_pop(L, 1);

  return 0;
}

auto surge::lua_sheet_set_indices(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 3) {
    glog<log_event::warning>("Function sheet_set_indices expected 3 arguments and instead got "
                             "{} arguments. Returning nil",
                             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!is_sprite(L, "sheet_set_indices")) {
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isnumber(L, 2)) {
    glog<log_event::warning>(
        "Function sheet_set_indices expected second argument to be a number. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isnumber(L, 3)) {
    glog<log_event::warning>(
        "Function sheet_set_indices expected third argument to be a number. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  // Data recovery
  auto vm_sprite_ptr{static_cast<sprite **>(lua_touserdata(L, 1))};
  auto sprite_ptr{*vm_sprite_ptr};
  const auto i{static_cast<GLfloat>(lua_tonumber(L, 2))},
      j{static_cast<GLfloat>(lua_tonumber(L, 3))};

  // Internal call
  sprite_ptr->sheet_set_indices(glm::vec2{i, j});

  lua_pop(L, 1);

  return 0;
}

auto surge::lua_sheet_set_offsets(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 3) {
    glog<log_event::warning>("Function sheet_set_offsets expected 3 arguments and instead got "
                             "{} arguments. Returning nil",
                             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!is_sprite(L, "sheet_set_offsets")) {
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isnumber(L, 2)) {
    glog<log_event::warning>(
        "Function sheet_set_offsets expected second argument to be a number. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isnumber(L, 3)) {
    glog<log_event::warning>(
        "Function sheet_set_offsets expected third argument to be a number. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  // Data recovery
  auto vm_sprite_ptr{static_cast<sprite **>(lua_touserdata(L, 1))};
  auto sprite_ptr{*vm_sprite_ptr};
  const auto x0{static_cast<GLfloat>(lua_tonumber(L, 2))},
      y0{static_cast<GLfloat>(lua_tonumber(L, 3))};

  // Internal call
  sprite_ptr->sheet_set_offset(glm::vec2{x0, y0});

  lua_pop(L, 1);

  return 0;
}

auto surge::lua_sheet_set_dimentions(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 3) {
    glog<log_event::warning>("Function sheet_set_dimentions expected 3 arguments and instead got "
                             "{} arguments. Returning nil",
                             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!is_sprite(L, "sheet_set_dimentions")) {
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isnumber(L, 2)) {
    glog<log_event::warning>(
        "Function sheet_set_dimentions expected second argument to be a number. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isnumber(L, 3)) {
    glog<log_event::warning>(
        "Function sheet_set_dimentions expected third argument to be a number. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  // Data recovery
  auto vm_sprite_ptr{static_cast<sprite **>(lua_touserdata(L, 1))};
  auto sprite_ptr{*vm_sprite_ptr};
  const auto Sw{static_cast<GLfloat>(lua_tonumber(L, 2))},
      Sh{static_cast<GLfloat>(lua_tonumber(L, 3))};

  // Internal call
  sprite_ptr->sheet_set_dimentions(glm::vec2{Sw, Sh});

  lua_pop(L, 1);

  return 0;
}