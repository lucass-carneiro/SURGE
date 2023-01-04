#include "lua/lua_wrappers.hpp"

#include "log.hpp"
#include "lua/lua_bindings.hpp"
#include "lua/lua_states.hpp"
#include "mesh/sprite.hpp"
#include "opengl/create_program.hpp"
#include "task_executor.hpp"
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

auto surge::lua_send_task_to(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 2) {
    glog<log_event::warning>("Function send_task_to expects 2 arguments: The target index and the "
                             "function to send Returning nil");
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isnumber(L, 1)) {
    glog<log_event::warning>(
        "Function send_task_to expects it's first argument to be a number. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isfunction(L, 2)) {
    glog<log_event::warning>(
        "Function send_task_to expects it's second argument to be a function. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  const auto target_vm_idx{static_cast<std::size_t>(lua_tointeger(L, 1))};

  if (target_vm_idx == 0 || target_vm_idx >= global_lua_states::get().size()) {
    glog<log_event::warning>("Cannot send to VM index {}. Returning nil", target_vm_idx);
    lua_pushnil(L);
    return 1;
  }

  // Get worker VM
  lua_State *worker{global_lua_states::get().at(target_vm_idx).get()};

  // Serialize the task
  lua_getglobal(L, "surge");
  lua_getfield(L, -1, "serialize");
  lua_pushvalue(L, 2);

  lua_call(L, 1, 1);

  // Copy serialization to worker
  const char *serialization_result{lua_tostring(L, -1)};
  lua_getglobal(worker, "loadstring");
  lua_pushstring(worker, serialization_result);
  lua_call(worker, 1, 1);
  lua_call(worker, 0, 1); // an extra call is necessary to get the actual callable task

  // Save the callable
  lua_setglobal(worker, "remote_task");

  // Clean the sender stack
  lua_pop(L, 2);

  // Return success
  lua_pushboolean(L, static_cast<lua_Boolean>(true));
  return 1;
}

auto surge::lua_run_task_at(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 1) {
    glog<log_event::warning>("Function run_task_at expects at 1 argument, the index of the VM to "
                             "run the task. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isnumber(L, 1)) {
    glog<log_event::warning>(
        "Function run_task_at expects it's argument to be a number. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  const auto target_vm_idx{static_cast<std::size_t>(lua_tointeger(L, 1))};

  if (target_vm_idx == 0 || target_vm_idx >= global_lua_states::get().size()) {
    glog<log_event::warning>("Cannot run task in VM index {}. Returning nil", target_vm_idx);
    lua_pushnil(L);
    return 1;
  }

  // Get worker VM
  lua_State *worker{global_lua_states::get().at(target_vm_idx).get()};

  // Schedule task in another thread
  global_task_executor::get().silent_async([=]() -> void {
    // Get remote task and ckeck for nil
    lua_getglobal(worker, "remote_task");

    if (lua_isnil(worker, -1)) {
      glog<log_event::warning>("No task loaded in VM {}.", target_vm_idx);
      lua_pushboolean(worker, static_cast<lua_Boolean>(false));
      lua_setglobal(worker, "remote_task_success");
      lua_pop(worker, 1);
      return;
    }

    // Call remote task.
    lua_pcall(worker, 0, 0, 0);

    lua_pushboolean(worker, static_cast<lua_Boolean>(true));
    lua_setglobal(worker, "remote_task_success");
  });

  lua_pushboolean(L, static_cast<lua_Boolean>(true));
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

void surge::glfw_key_callback(GLFWwindow *, int key, int, int action, int mods) noexcept {

  // Recover main VM state
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
  return 2;
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