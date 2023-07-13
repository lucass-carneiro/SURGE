#include "allocator.hpp"
#include "entities/animated_sprite.hpp"
#include "logging_system/logging_system.hpp"
#include "lua/lua_wrappers.hpp"

auto surge::lua_new_animated_sprite(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 9) {
    log_warn("Function new_animated_sprite expected 12 arguments and instead got {} "
             "arguments. Returning nil",
             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!(lua_isstring(L, 1) || lua_isstring(L, 2))) {
    log_warn("Fucntion new_animated_sprite expects arguents 1 and 2 to be strings. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isnumber(L, 3)) {
    log_warn("Fucntion new_animated_sprite expects argument 3 to be an integer. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  for (int i = 4; i <= 9; i++) {
    if (!lua_isnumber(L, i)) {
      log_warn("Function new_animated_sprite expected argument {} to be a number. Returning nil",
               i);
      lua_pushnil(L);
      return 1;
    }
  }

  // Argument extraction
  const char *sheet_path_str{lua_tostring(L, 1)};
  const char *sad_path_str{lua_tostring(L, 2)};
  const auto anim_idx{static_cast<std::uint32_t>(lua_tointeger(L, 3))};

  const auto pos_x{static_cast<float>(lua_tonumber(L, 4))};
  const auto pos_y{static_cast<float>(lua_tonumber(L, 5))};
  const auto pos_z{static_cast<float>(lua_tonumber(L, 6))};

  const auto scale_x{static_cast<float>(lua_tonumber(L, 7))};
  const auto scale_y{static_cast<float>(lua_tonumber(L, 8))};
  const auto scale_z{static_cast<float>(lua_tonumber(L, 9))};

  // Internal call
  auto animated_sprite_buffer{mi_malloc(sizeof(animated_sprite))};
  animated_sprite *animated_sprite_ptr{new (animated_sprite_buffer) animated_sprite(
      sheet_path_str, sad_path_str, anim_idx, glm::vec3{pos_x, pos_y, pos_z},
      glm::vec3{scale_x, scale_y, scale_z})};

  // Pass this pointer to the Lua VM as userdata
  auto vm_animated_sprite_ptr{static_cast<animated_sprite **>(lua_newuserdata(L, sizeof(void *)))};
  *vm_animated_sprite_ptr = animated_sprite_ptr;

  lua_getglobal(L, "surge");
  lua_getfield(L, -1, "animated_sprite");
  lua_getfield(L, -1, "animated_sprite_meta");
  lua_setmetatable(L, -4);
  lua_pop(L, 2);

  return 1;
}

auto surge::lua_drop_animated_sprite(lua_State *L) noexcept -> int {
  // Data recovery
  auto vm_animated_sprite_ptr{static_cast<animated_sprite **>(lua_touserdata(L, 1))};
  auto animated_sprite_ptr{*vm_animated_sprite_ptr};

  // Data cleanup
  mi_free(animated_sprite_ptr);

  return 0;
}

[[nodiscard]] static auto is_animated_sprite(lua_State *L, const char *func_name) noexcept -> bool {
  if (!lua_isuserdata(L, 1)) {
    log_warn("Function {} expected 1 user data argument. Returning nil", func_name);
    return false;
  }

  if (!lua_getmetatable(L, 1)) {
    log_warn("User data does not have a metatable");
    return false;
  }

  lua_getfield(L, -1, "__name");
  const auto name{lua_tostring(L, -1)};

  if (std::strcmp(name, "surge::animated_sprite") != 0) {
    log_warn("Expected surge::animated_sprite userdata and recieved {}", name);
    return false;
  }

  lua_pop(L, 2);
  return true;
}

auto surge::lua_draw_animated_sprite(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 1) {
    log_warn("Function draw_animated_sprite expected 1 user data arguments and instead got "
             "{} arguments. Returning nil",
             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!is_animated_sprite(L, "draw_animated_sprite")) {
    lua_pushnil(L);
    return 1;
  }

  // Data recovery
  auto vm_animated_sprite_ptr{static_cast<animated_sprite **>(lua_touserdata(L, 1))};
  auto animated_sprite_ptr{*vm_animated_sprite_ptr};

  // Internal call
  animated_sprite_ptr->draw();

  lua_pop(L, 1);

  return 0;
}

auto surge::lua_move_animated_sprite(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 4) {
    log_warn("Function move_animated_sprite expected 4 arguments and instead got "
             "{} arguments. Returning nil",
             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!is_animated_sprite(L, "move_animated_sprite")) {
    lua_pushnil(L);
    return 1;
  }

  for (int i = 2; i <= 4; i++) {
    if (!lua_isnumber(L, i)) {
      log_warn("Function move_animated_sprite expected argument {} to be a number. Returning nil",
               i);
      lua_pushnil(L);
      return 1;
    }
  }

  // Data recovery
  auto vm_animated_sprite_ptr{static_cast<animated_sprite **>(lua_touserdata(L, 1))};
  auto animated_sprite_ptr{*vm_animated_sprite_ptr};
  const auto mx{static_cast<float>(lua_tonumber(L, 2))}, my{static_cast<float>(lua_tonumber(L, 3))},
      mz{static_cast<float>(lua_tonumber(L, 4))};

  // Internal call
  animated_sprite_ptr->move(glm::vec3{mx, my, mz});

  lua_pop(L, 1);

  return 0;
}

auto surge::lua_scale_animated_sprite(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 4) {
    log_warn("Function scale_animated_sprite expected 4 arguments and instead got "
             "{} arguments. Returning nil",
             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!is_animated_sprite(L, "scale_animated_sprite")) {
    lua_pushnil(L);
    return 1;
  }

  for (int i = 2; i <= 4; i++) {
    if (!lua_isnumber(L, i)) {
      log_warn("Function scale_animated_sprite expected argument {} to be a number. Returning nil",
               i);
      lua_pushnil(L);
      return 1;
    }
  }

  // Data recovery
  auto vm_animated_sprite_ptr{static_cast<animated_sprite **>(lua_touserdata(L, 1))};
  auto animated_sprite_ptr{*vm_animated_sprite_ptr};
  const auto sx{static_cast<float>(lua_tonumber(L, 2))}, sy{static_cast<float>(lua_tonumber(L, 3))},
      sz{static_cast<float>(lua_tonumber(L, 4))};

  // Internal call
  animated_sprite_ptr->scale(glm::vec3{sx, sy, sz});

  lua_pop(L, 1);

  return 0;
}

auto surge::lua_update_animated_sprite(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 3) {
    log_warn("Function update expected 3 arguments and instead got "
             "{} arguments. Returning nil",
             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!is_animated_sprite(L, "update")) {
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isnumber(L, 2)) {
    log_warn("Function update expected argument 2 to be a number. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isnumber(L, 2)) {
    log_warn("Function update expected argument 3 to be a number. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  // Data recovery
  auto vm_animated_sprite_ptr{static_cast<animated_sprite **>(lua_touserdata(L, 1))};
  auto animated_sprite_ptr{*vm_animated_sprite_ptr};
  const auto dt{lua_tonumber(L, 2)};
  const auto delay{lua_tonumber(L, 3)};

  // Internal call
  animated_sprite_ptr->update(dt, delay);

  lua_pop(L, 1);

  return 0;
}

auto surge::lua_change_animated_sprite_anim(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 3) {
    log_warn("Function change_animation_to expected 3 arguments and instead got "
             "{} arguments. Returning nil",
             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!is_animated_sprite(L, "change_animation_to")) {
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isnumber(L, 2)) {
    log_warn("Function change_animation_to expected argument 2 to be a integer. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isboolean(L, 3)) {
    log_warn("Function change_animation_to expected argument 3 to be a boolean. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  // Data recovery
  auto vm_animated_sprite_ptr{static_cast<animated_sprite **>(lua_touserdata(L, 1))};
  auto animated_sprite_ptr{*vm_animated_sprite_ptr};
  const auto index{static_cast<std::uint32_t>(lua_tonumber(L, 2))};
  const auto loops{static_cast<bool>(lua_toboolean(L, 3))};

  // Internal call
  animated_sprite_ptr->change_current_animation_to(index, loops);

  lua_pop(L, 1);

  return 0;
}

auto surge::lua_animated_sprite_toggle_h_flip(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 1) {
    log_warn("Function toggle_h_flip expected 1 argument and instead got "
             "{} arguments. Returning nil",
             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!is_animated_sprite(L, "toggle_h_flip")) {
    lua_pushnil(L);
    return 1;
  }

  // Data recovery
  auto vm_animated_sprite_ptr{static_cast<animated_sprite **>(lua_touserdata(L, 1))};
  auto animated_sprite_ptr{*vm_animated_sprite_ptr};

  // Internal call
  animated_sprite_ptr->toggle_h_flip();

  return 0;
}

auto surge::lua_animated_sprite_toggle_v_flip(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 1) {
    log_warn("Function toggle_v_flip expected 1 argument and instead got "
             "{} arguments. Returning nil",
             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!is_animated_sprite(L, "toggle_v_flip")) {
    lua_pushnil(L);
    return 1;
  }

  // Data recovery
  auto vm_animated_sprite_ptr{static_cast<animated_sprite **>(lua_touserdata(L, 1))};
  auto animated_sprite_ptr{*vm_animated_sprite_ptr};

  // Internal call
  animated_sprite_ptr->toggle_v_flip();

  return 0;
}