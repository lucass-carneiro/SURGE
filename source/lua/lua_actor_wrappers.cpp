#include "allocator.hpp"
#include "entities/actor.hpp"
#include "logging_system/logging_system.hpp"
#include "lua/lua_wrappers.hpp"

auto surge::lua_new_actor(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 12) {
    log_warn("Function new_actor expected 12 arguments and instead got {} "
             "arguments. Returning nil",
             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!(lua_isstring(L, 1) || lua_isstring(L, 2))) {
    log_warn("Fucntion new_actor expects arguents 1 and 2 to be strings. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isnumber(L, 3)) {
    log_warn("Fucntion new_actor expects argument 3 to be an integer. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  for (int i = 4; i <= 12; i++) {
    if (!lua_isnumber(L, i)) {
      log_warn("Function new_actor expected argument {} to be a number. Returning nil", i);
      lua_pushnil(L);
      return 1;
    }
  }

  // Argument extraction
  const char *sheet_path_str{lua_tostring(L, 1)};
  const char *sad_path_str{lua_tostring(L, 2)};
  const auto anim_idx{static_cast<std::uint32_t>(lua_tointeger(L, 3))};

  const auto anchor_x{static_cast<float>(lua_tonumber(L, 4))};
  const auto anchor_y{static_cast<float>(lua_tonumber(L, 5))};
  const auto anchor_z{static_cast<float>(lua_tonumber(L, 6))};

  const auto pos_x{static_cast<float>(lua_tonumber(L, 7))};
  const auto pos_y{static_cast<float>(lua_tonumber(L, 8))};
  const auto pos_z{static_cast<float>(lua_tonumber(L, 9))};

  const auto scale_x{static_cast<float>(lua_tonumber(L, 10))};
  const auto scale_y{static_cast<float>(lua_tonumber(L, 11))};
  const auto scale_z{static_cast<float>(lua_tonumber(L, 12))};

  // Internal call
  auto actor_buffer{mi_malloc(sizeof(actor))};
  actor *actor_ptr{new (actor_buffer) actor(
      sheet_path_str, sad_path_str, anim_idx, glm::vec3{anchor_x, anchor_y, anchor_z},
      glm::vec3{pos_x, pos_y, pos_z}, glm::vec3{scale_x, scale_y, scale_z})};

  // Pass this pointer to the Lua VM as userdata
  auto vm_actor_ptr{static_cast<actor **>(lua_newuserdata(L, sizeof(void *)))};
  *vm_actor_ptr = actor_ptr;

  lua_getglobal(L, "surge");
  lua_getfield(L, -1, "actor");
  lua_getfield(L, -1, "actor_meta");
  lua_setmetatable(L, -4);
  lua_pop(L, 2);

  return 1;
}

auto surge::lua_drop_actor(lua_State *L) noexcept -> int {
  // Data recovery
  auto vm_actor_ptr{static_cast<actor **>(lua_touserdata(L, 1))};
  auto actor_ptr{*vm_actor_ptr};

  // Data cleanup
  mi_free(actor_ptr);

  return 0;
}

[[nodiscard]] static auto is_actor(lua_State *L, const char *func_name) noexcept -> bool {
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

  if (std::strcmp(name, "surge::actor") != 0) {
    log_warn("Expected surge::actor userdata and recieved {}", name);
    return false;
  }

  lua_pop(L, 2);
  return true;
}

auto surge::lua_draw_actor(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 1) {
    log_warn("Function draw_actor expected 1 user data arguments and instead got "
             "{} arguments. Returning nil",
             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!is_actor(L, "draw_actor")) {
    lua_pushnil(L);
    return 1;
  }

  // Data recovery
  auto vm_actor_ptr{static_cast<actor **>(lua_touserdata(L, 1))};
  auto actor_ptr{*vm_actor_ptr};

  // Internal call
  actor_ptr->draw();

  lua_pop(L, 1);

  return 0;
}

auto surge::lua_move_actor(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 4) {
    log_warn("Function move_actor expected 4 arguments and instead got "
             "{} arguments. Returning nil",
             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!is_actor(L, "move_actor")) {
    lua_pushnil(L);
    return 1;
  }

  for (int i = 2; i <= 4; i++) {
    if (!lua_isnumber(L, i)) {
      log_warn("Function move_actor expected argument {} to be a number. Returning nil", i);
      lua_pushnil(L);
      return 1;
    }
  }

  // Data recovery
  auto vm_actor_ptr{static_cast<actor **>(lua_touserdata(L, 1))};
  auto actor_ptr{*vm_actor_ptr};
  const auto mx{static_cast<float>(lua_tonumber(L, 2))}, my{static_cast<float>(lua_tonumber(L, 3))},
      mz{static_cast<float>(lua_tonumber(L, 4))};

  // Internal call
  actor_ptr->move(glm::vec3{mx, my, mz});

  lua_pop(L, 1);

  return 0;
}

auto surge::lua_scale_actor(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 4) {
    log_warn("Function scale_actor expected 4 arguments and instead got "
             "{} arguments. Returning nil",
             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!is_actor(L, "scale_actor")) {
    lua_pushnil(L);
    return 1;
  }

  for (int i = 2; i <= 4; i++) {
    if (!lua_isnumber(L, i)) {
      log_warn("Function scale_actor expected argument {} to be a number. Returning nil", i);
      lua_pushnil(L);
      return 1;
    }
  }

  // Data recovery
  auto vm_actor_ptr{static_cast<actor **>(lua_touserdata(L, 1))};
  auto actor_ptr{*vm_actor_ptr};
  const auto sx{static_cast<float>(lua_tonumber(L, 2))}, sy{static_cast<float>(lua_tonumber(L, 3))},
      sz{static_cast<float>(lua_tonumber(L, 4))};

  // Internal call
  actor_ptr->scale(glm::vec3{sx, sy, sz});

  lua_pop(L, 1);

  return 0;
}

auto surge::lua_update_actor(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 3) {
    log_warn("Function update expected 3 arguments and instead got "
             "{} arguments. Returning nil",
             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!is_actor(L, "update")) {
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isnumber(L, 2)) {
    log_warn("Function update expected argument 2 to be a number. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isnumber(L, 3)) {
    log_warn("Function update expected argument 3 to be a number. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  // Data recovery
  auto vm_actor_ptr{static_cast<actor **>(lua_touserdata(L, 1))};
  auto actor_ptr{*vm_actor_ptr};
  const auto dt{lua_tonumber(L, 2)};
  const auto delay{lua_tonumber(L, 3)};

  // Internal call
  actor_ptr->update(dt, delay);

  lua_pop(L, 1);

  return 0;
}

auto surge::lua_change_actor_anim(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 3) {
    log_warn("Function change_animation_to expected 3 arguments and instead got "
             "{} arguments. Returning nil",
             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!is_actor(L, "change_animation_to")) {
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
  auto vm_actor_ptr{static_cast<actor **>(lua_touserdata(L, 1))};
  auto actor_ptr{*vm_actor_ptr};
  const auto index{static_cast<std::uint32_t>(lua_tonumber(L, 2))};
  const auto loops{static_cast<bool>(lua_toboolean(L, 3))};

  // Internal call
  actor_ptr->change_current_animation_to(index, loops);

  lua_pop(L, 1);

  return 0;
}

auto surge::lua_get_actor_anchor_coords(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 1) {
    log_warn("Function get_anchor_coords expected 1 argument and instead got "
             "{} arguments. Returning nil",
             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!is_actor(L, "get_anchor_coords")) {
    lua_pushnil(L);
    return 1;
  }

  // Data recovery
  auto vm_actor_ptr{static_cast<actor **>(lua_touserdata(L, 1))};
  auto actor_ptr{*vm_actor_ptr};

  // Internal call
  const auto anchor_coords{actor_ptr->get_anchor_coordinates()};

  lua_pushnumber(L, anchor_coords[0]);
  lua_pushnumber(L, anchor_coords[1]);
  lua_pushnumber(L, anchor_coords[2]);

  return 3;
}

auto surge::lua_actor_toggle_h_flip(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 1) {
    log_warn("Function toggle_h_flip expected 1 argument and instead got "
             "{} arguments. Returning nil",
             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!is_actor(L, "toggle_h_flip")) {
    lua_pushnil(L);
    return 1;
  }

  // Data recovery
  auto vm_actor_ptr{static_cast<actor **>(lua_touserdata(L, 1))};
  auto actor_ptr{*vm_actor_ptr};

  // Internal call
  actor_ptr->toggle_h_flip();

  return 0;
}

auto surge::lua_actor_toggle_v_flip(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 1) {
    log_warn("Function toggle_v_flip expected 1 argument and instead got "
             "{} arguments. Returning nil",
             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!is_actor(L, "toggle_v_flip")) {
    lua_pushnil(L);
    return 1;
  }

  // Data recovery
  auto vm_actor_ptr{static_cast<actor **>(lua_touserdata(L, 1))};
  auto actor_ptr{*vm_actor_ptr};

  // Internal call
  actor_ptr->toggle_v_flip();

  return 0;
}