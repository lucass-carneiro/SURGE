#include "lua/lua_states.hpp"

#include "allocator.hpp"
#include "job_system/job_system.hpp"
#include "lua/lua_utils.hpp"

surge::lua_states::state_vec_t surge::lua_states::state_array = state_vec_t{eastl_allocator{}};

auto surge::lua_states::init() noexcept -> bool {
  log_info("Starting up Lua states");

  const auto num_threads{std::thread::hardware_concurrency()};

  // Step 1: Allocate memory for the array of state pointers
  state_array.reserve(num_threads);

  // Setep 2: Allocate each state and store the pointer. Do initializations
  for (unsigned int i = 0; i < num_threads; i++) {
    auto L{luaL_newstate()};

    if (L == nullptr) {
      log_error("Failed to initialize lua state {}", i);
      return false;
    }

    luaL_openlibs(L);
    luaopen_jit(L);
    luaopen_ffi(L);
    luaJIT_setmode(L, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_ON);

    job_system::executor.async(lua_add_engine_context, L, i);

    state_array.push_back(lua_state_ptr{L, lua_close});
  }

  job_system::executor.wait_for_all();

  return true;
}

auto surge::lua_states::configure(const char *path) noexcept -> bool {
  for (auto &lua_state_ptr : state_array) {
    if (!do_file_at(lua_state_ptr.get(), path)) {
      return false;
    }
  }

  return true;
}

auto surge::lua_states::at(std::size_t i) noexcept -> lua_state_ptr & {
  try {
    return state_array.at(i);
  } catch (const std::exception &e) {
    log_error("Uanble to acess global lua VM array at index {}: {}. Returning main thread VM.", i,
              e.what());
  }
  return state_array[0];
}