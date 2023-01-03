#include "lua/lua_states.hpp"

#include "lua/lua_utils.hpp"
#include "thread_allocators.hpp"

void surge::global_lua_states::init() noexcept {
  glog<log_event::message>("Starting up Lua states");

  const auto num_threads{global_thread_allocators::get().get_num_threads()};

  // Step 1: Allocate memory for the array of state pointers
  state_array.reserve(num_threads);

  // Setep 2: Allocate each state and store the pointer. Do initializations
  for (unsigned int i = 0; i < num_threads; i++) {
    state_array.push_back(lua_state_ptr{luaL_newstate(), lua_close});

    // Initialize state with libs
    luaL_openlibs(state_array[i].get());

    // Turn JIT on globally
    luaJIT_setmode(state_array[i].get(), 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_ON);

    // Add engine context
    push_engine_config_at(i);
  }
}

auto surge::global_lua_states::configure(const std::filesystem::path &path) noexcept -> bool {
  for (std::size_t i = 0; i < state_array.size(); i++) {
    const auto succes{do_file_at(i, path)};
    if (!succes) {
      return false;
    }
  }

  return true;
}

surge::global_lua_states::~global_lua_states() noexcept {
  glog<log_event::message>("Closing Lua states");
}

auto surge::global_lua_states::at(std::size_t i) noexcept -> lua_state_ptr & {
  try {
    return state_array.at(i);
  } catch (const std::exception &e) {
    glog<log_event::error>(
        "Uanble to acess global lua VM array at index {}: {}. Returning main thread VM.", i,
        e.what());
  }
  return state_array.back();
}

auto surge::global_lua_states::back() noexcept -> lua_state_ptr & { return state_array.back(); }