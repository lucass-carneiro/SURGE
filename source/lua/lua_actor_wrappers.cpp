#include "entities/actor.hpp"
#include "lua/lua_wrappers.hpp"
#include "thread_allocators.hpp"

auto surge::lua_new_actor(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 2) {
    glog<log_event::warning>("Function new_actor expected 2 arguments and instead got {} "
                             "arguments. Returning nil",
                             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!(lua_isstring(L, 1) || lua_isstring(L, 2))) {
    glog<log_event::warning>("Fucntion new_actor expects 2 string arguments. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  // Argument extraction
  const char *sheet_path_str{lua_tostring(L, 1)};
  const char *sad_path_str{lua_tostring(L, 2)};

  // VM index recovery
  lua_getglobal(L, "surge");
  lua_getfield(L, -1, "vm_index");
  const auto vm_index{static_cast<std::size_t>(lua_tointeger(L, -1))};
  lua_pop(L, 2);

  // Internal call
  auto actor_buffer{global_thread_allocators::get().at(vm_index).get()->malloc(sizeof(actor))};
  actor *actor_ptr{new (actor_buffer) actor(global_thread_allocators::get().at(vm_index).get(),
                                            sheet_path_str, sad_path_str)};

  // Pass this pointer to the Lua VM as userdata
  auto vm_actor_ptr{static_cast<actor **>(lua_newuserdata(L, sizeof(void *)))};
  *vm_actor_ptr = actor_ptr;

  lua_getglobal(L, "surge");
  lua_getfield(L, -1, "actor_meta");
  lua_setmetatable(L, -3);
  lua_pop(L, 1);

  return 1;
}

auto surge::lua_drop_actor(lua_State *L) noexcept -> int {
  // Data recovery
  auto vm_actor_ptr{static_cast<actor **>(lua_touserdata(L, 1))};
  auto actor_ptr{*vm_actor_ptr};

  // VM index recovery
  lua_getglobal(L, "surge");
  lua_getfield(L, -1, "vm_index");
  const auto vm_index{static_cast<std::size_t>(lua_tointeger(L, -1))};
  lua_pop(L, 2);

  // Data cleanup
  actor_ptr->drop_sad_file(global_thread_allocators::get().at(vm_index).get());
  global_thread_allocators::get().at(vm_index)->free(actor_ptr);

  return 0;
}

[[nodiscard]] auto is_actor(lua_State *L, const char *func_name) noexcept -> bool {
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

  if (std::strcmp(name, "surge::actor") != 0) {
    surge::glog<surge::log_event::warning>("Expected surge::actor userdata and recieved {}", name);
    return false;
  }

  lua_pop(L, 2);
  return true;
}

auto surge::lua_draw_actor(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 1) {
    glog<log_event::warning>("Function draw_actor expected 1 user data arguments and instead got "
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

auto surge::lua_set_actor_animation(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 2) {
    glog<log_event::warning>("Function set_actor_animation expected 2 arguments and instead got "
                             "{} arguments. Returning nil",
                             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!is_actor(L, "set_actor_animation")) {
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isnumber(L, 2)) {
    glog<log_event::warning>(
        "Function set_actor_animation expected argument 2 to be a number. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  // Data recovery
  auto vm_actor_ptr{static_cast<actor **>(lua_touserdata(L, 1))};
  auto actor_ptr{*vm_actor_ptr};
  const auto index{static_cast<std::uint32_t>(lua_tonumber(L, 2))};

  // Internal call
  actor_ptr->select_animation(index);

  lua_pop(L, 1);

  return 0;
}

auto surge::lua_set_actor_geometry(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 7) {
    glog<log_event::warning>("Function set_actor_geometry expected 7 arguments and instead got "
                             "{} arguments. Returning nil",
                             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!is_actor(L, "set_actor_geometry")) {
    lua_pushnil(L);
    return 1;
  }

  for (int i = 2; i <= 7; i++) {
    if (!lua_isnumber(L, i)) {
      glog<log_event::warning>(
          "Function set_actor_geometry expected argument {} to be a number. Returning nil", i);
      lua_pushnil(L);
      return 1;
    }
  }

  // Data recovery
  auto vm_actor_ptr{static_cast<actor **>(lua_touserdata(L, 1))};
  auto actor_ptr{*vm_actor_ptr};
  const auto mx{static_cast<float>(lua_tonumber(L, 2))}, my{static_cast<float>(lua_tonumber(L, 3))},
      mz{static_cast<float>(lua_tonumber(L, 4))}, sx{static_cast<float>(lua_tonumber(L, 5))},
      sy{static_cast<float>(lua_tonumber(L, 6))}, sz{static_cast<float>(lua_tonumber(L, 7))};

  // Internal call
  actor_ptr->set_geometry(glm::vec3{mx, my, mz}, glm::vec3{sx, sy, sz});

  lua_pop(L, 1);

  return 0;
}

auto surge::lua_set_actor_position(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 5) {
    glog<log_event::warning>("Function set_actor_position expected 5 arguments and instead got "
                             "{} arguments. Returning nil",
                             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!is_actor(L, "set_actor_position")) {
    lua_pushnil(L);
    return 1;
  }

  for (int i = 2; i <= 5; i++) {
    if (!lua_isnumber(L, i)) {
      glog<log_event::warning>(
          "Function set_actor_position expected argument {} to be a number. Returning nil", i);
      lua_pushnil(L);
      return 1;
    }
  }

  // Data recovery
  auto vm_actor_ptr{static_cast<actor **>(lua_touserdata(L, 1))};
  auto actor_ptr{*vm_actor_ptr};
  const auto mx{static_cast<float>(lua_tonumber(L, 2))}, my{static_cast<float>(lua_tonumber(L, 3))},
      mz{static_cast<float>(lua_tonumber(L, 4))}, s{static_cast<float>(lua_tonumber(L, 5))};

  // Internal call
  actor_ptr->set_position(glm::vec3{mx, my, mz}, s);

  lua_pop(L, 1);

  return 0;
}

auto surge::lua_set_actor_anchor_point(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 4) {
    glog<log_event::warning>("Function set_actor_anchor_point expected 4 arguments and instead got "
                             "{} arguments. Returning nil",
                             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!is_actor(L, "set_actor_anchor_point")) {
    lua_pushnil(L);
    return 1;
  }

  for (int i = 2; i <= 4; i++) {
    if (!lua_isnumber(L, i)) {
      glog<log_event::warning>(
          "Function set_actor_anchor_point expected argument {} to be a number. Returning nil", i);
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
  actor_ptr->set_anchor_point(glm::vec3{mx, my, mz});

  lua_pop(L, 1);

  return 0;
}

auto surge::lua_advance_actor_frame(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 1) {
    glog<log_event::warning>("Function advance_actor_frame expected 1 arguments and instead got "
                             "{} arguments. Returning nil",
                             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!is_actor(L, "advance_actor_frame")) {
    lua_pushnil(L);
    return 1;
  }

  // Data recovery
  auto vm_actor_ptr{static_cast<actor **>(lua_touserdata(L, 1))};
  auto actor_ptr{*vm_actor_ptr};

  // Internal call
  actor_ptr->advance_frame();

  lua_pop(L, 1);

  return 0;
}

auto surge::lua_move_actor(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 4) {
    glog<log_event::warning>("Function move_actor expected 4 arguments and instead got "
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
      glog<log_event::warning>(
          "Function move_actor expected argument {} to be a number. Returning nil", i);
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
    glog<log_event::warning>("Function scale_actor expected 4 arguments and instead got "
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
      glog<log_event::warning>(
          "Function scale_actor expected argument {} to be a number. Returning nil", i);
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