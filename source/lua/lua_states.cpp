#include "lua/lua_states.hpp"

#include "allocator.hpp"
#include "lua/lua_utils.hpp"
#include "task_executor.hpp"

auto surge::global_lua_states::init() noexcept -> bool {
  surge::log_info("Starting up Lua states");

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
    luaJIT_setmode(L, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_ON);

    job_system::get().executor().async(lua_add_engine_context, L, i);

    state_array.push_back(lua_state_ptr{L, lua_close});
  }

  job_system::get().executor().wait_for_all();
  return true;
}

auto surge::global_lua_states::configure(const char *path) noexcept -> bool {
  for (auto &lua_state_ptr : state_array) {
    if (!do_file_at(lua_state_ptr.get(), path)) {
      return false;
    }
  }

  return true;
}

surge::global_lua_states::~global_lua_states() noexcept { surge::log_info("Closing Lua states"); }

auto surge::global_lua_states::at(std::size_t i) noexcept -> lua_state_ptr & {
  try {
    return state_array.at(i);
  } catch (const std::exception &e) {
    log_error("Uanble to acess global lua VM array at index {}: {}. Returning main thread VM.", i,
              e.what());
  }
  return state_array[0];
}

auto surge::global_lua_states::back() noexcept -> lua_state_ptr & { return state_array.back(); }