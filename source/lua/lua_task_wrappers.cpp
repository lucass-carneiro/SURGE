#include "log.hpp"
#include "lua/lua_states.hpp"
#include "lua/lua_utils.hpp"
#include "lua/lua_wrappers.hpp"
#include "task_executor.hpp"

auto surge::lua_send_task_to(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 2) {
    log_warn("Function send_task_to expects 2 arguments: The target index and the "
             "function to send Returning nil");
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isnumber(L, 1)) {
    log_warn("Function send_task_to expects it's first argument to be a number. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isfunction(L, 2)) {
    log_warn("Function send_task_to expects it's second argument to be a function. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  const auto target_vm_idx{static_cast<std::size_t>(lua_tointeger(L, 1))};

  if (target_vm_idx == 0 || target_vm_idx >= global_lua_states::get().size()) {
    log_warn("Cannot send to VM index {}. Returning nil", target_vm_idx);
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
    log_warn("Function run_task_at expects at 1 argument, the index of the VM to "
             "run the task. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isnumber(L, 1)) {
    log_warn("Function run_task_at expects it's argument to be a number. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  const auto target_vm_idx{static_cast<std::size_t>(lua_tointeger(L, 1))};

  if (target_vm_idx == 0 || target_vm_idx >= global_lua_states::get().size()) {
    log_warn("Cannot run task in VM index {}. Returning nil", target_vm_idx);
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
      log_warn("No task loaded in VM {}.", target_vm_idx);
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