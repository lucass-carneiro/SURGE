#include "lua_vm.hpp"

#include "allocators/global_allocators.hpp"
#include "file.hpp"
#include "log.hpp"
#include "safe_ops.hpp"
#include "task_executor.hpp"
#include "thread_allocators.hpp"

#include <exception>
#include <fmt/core.h>

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

void surge::push_engine_config_at(std::size_t i) noexcept {
  auto L = global_lua_states::get().at(i).get();

  // begin surge table
  lua_newtable(L);

  // TODO: Warning! This is potentially very dangerous. What happens if the user changes this
  // number? Maybe this can be made more secure.
  add_table_field<lua_String, lua_Integer>(L, "vm_index", safe_cast<long>(i).value_or(0));

  add_table_field<lua_String, lua_Integer>(L, "window_width", 800);
  add_table_field<lua_String, lua_Integer>(L, "window_height", 600);
  add_table_field<lua_String, lua_String>(L, "window_name", "SURGE window");
  add_table_field<lua_String, lua_Boolean>(L, "windowed", false);
  add_table_field<lua_String, lua_Integer>(L, "window_monitor_index", 1);

  // begin clear_color array
  lua_newtable(L);
  push_array(L, std::array<lua_Number, 4>{0.0, 0.0, 0.0, 1.0});
  lua_setfield(L, -2, "clear_color");
  // end clear_color table

  add_table_field<lua_String, lua_CFunction>(L, "log_message", lua_log_message);
  add_table_field<lua_String, lua_CFunction>(L, "log_warning", lua_log_warning);
  add_table_field<lua_String, lua_CFunction>(L, "log_error", lua_log_error);
  add_table_field<lua_String, lua_CFunction>(L, "log_memory", lua_log_memory);

  lua_setglobal(L, "surge");
  // end surge table
}

/*struct lua_engine_config {
  lua_Integer window_width;
  lua_Integer window_height;
  lua_String window_name;
  lua_Boolean windowed;
  lua_Integer window_monitor_index;
  std::array<lua_Number, 4> clear_color;
};*/

auto surge::get_lua_engine_config(lua_State *L) noexcept -> std::optional<lua_engine_config> {
  lua_engine_config config{};

  const auto stack_top{lua_gettop(L)};

  lua_getglobal(L, "surge");

  lua_getfield(L, -1, "window_width");
  if (!lua_isnumber(L, -1)) {
    glog<log_event::error>("The value stored in the field window_width is not a number");
    lua_settop(L, stack_top);
    return {};
  } else {
    config.window_width = lua_tointeger(L, -1);
    lua_pop(L, 1);
  }

  lua_getfield(L, -1, "window_height");
  if (!lua_isnumber(L, -1)) {
    glog<log_event::error>("The value stored in the field window_height is not a number");
    lua_settop(L, stack_top);
    return {};
  } else {
    config.window_height = lua_tointeger(L, -1);
    lua_pop(L, 1);
  }

  lua_getfield(L, -1, "window_name");
  if (!lua_isstring(L, -1)) {
    glog<log_event::error>("The value stored in the field window_name is not a string");
    lua_settop(L, stack_top);
    return {};
  } else {
    config.window_name = lua_tostring(L, -1);
    lua_pop(L, 1);
  }

  lua_getfield(L, -1, "windowed");
  if (!lua_isboolean(L, -1)) {
    glog<log_event::error>("The value stored in the field windowed is not a boolean");
    lua_settop(L, stack_top);
    return {};
  } else {
    config.windowed = static_cast<lua_Boolean>(lua_toboolean(L, -1));
    lua_pop(L, 1);
  }

  lua_getfield(L, -1, "window_monitor_index");
  if (!lua_isnumber(L, -1)) {
    glog<log_event::error>("The value stored in the field window_monitor_index is not a number");
    lua_settop(L, stack_top);
    return {};
  } else {
    config.window_monitor_index = lua_tointeger(L, -1);
    lua_pop(L, 1);
  }

  lua_getfield(L, -1, "clear_color");
  if (!lua_istable(L, -1)) {
    glog<log_event::error>("The value stored in the field clear_color is not a table");
    lua_settop(L, stack_top);
    return {};

  } else {

    for (int i = 1; auto &color : config.clear_color) {
      lua_rawgeti(L, -1, i);

      if (lua_isnoneornil(L, -1) || !lua_isnumber(L, -1)) {
        glog<log_event::error>(
            "The value stored in the field clear_color[{}] is nill, none or not a number", i);
        lua_settop(L, stack_top);
        return {};
      }

      color = lua_tonumber(L, -1);
      lua_pop(L, 1);
      i++;
    }

    lua_pop(L, 1);
  }

  lua_settop(L, stack_top);

  return config;
}

struct file_handle {
  surge::load_file_return_t opt_file_span;
  bool file_read{false};
};

auto surge::lua_reader(lua_State *, void *user_data, std::size_t *chunck_size) noexcept -> const
    char * {
  auto handle{static_cast<file_handle *>(user_data)};

  if (!handle->file_read) {
    *chunck_size = handle->opt_file_span.value().size();
    handle->file_read = true;
    return static_cast<const char *>(static_cast<void *>(handle->opt_file_span.value().data()));
  } else {
    *chunck_size = 0;
    return nullptr;
  }
}

auto surge::lua_message_handler(lua_State *L) noexcept -> int {
  const char *msg = lua_tostring(L, 1);

  // is error object not a string?
  if (msg == nullptr) {

    // does it have a metamethod that produces a string?
    if (luaL_callmeta(L, 1, "__tostring") && lua_type(L, -1) == LUA_TSTRING)

      // that is the message
      return 1;
    else
      // I have no other choice here.
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
      msg = lua_pushfstring(L, "(error object is a %s value)", luaL_typename(L, 1));
  }

  // append a standard traceback
  luaL_traceback(L, L, msg, 1);

  // return the traceback
  return 1;
}

auto surge::do_file_at(std::size_t i, const std::filesystem::path &path) noexcept -> bool {
  // See example implementation from the Lua interpreter here:
  // https://www.lua.org/source/5.4/lua.c.html#msghandler

  glog<log_event::message>("Doing Lua script {} using index {}", path.c_str(), i);
  // luaL_dofile is  (luaL_loadfile(L, filename) || lua_pcall(L, 0, LUA_MULTRET, 0))

  auto L{global_lua_states::get().at(i).get()};
  auto &alloc{global_thread_allocators::get().at(i)};

  // Step 1: load file into the thread's stack and construct a file_handle object to pass to
  // lua_reader
  file_handle handle{load_file(alloc.get(), path, ".lua"), false};
  if (!handle.opt_file_span) {
    return false;
  }

  // Step 2: Load Lua chunk
  const auto lua_laod_stats{lua_load(L, &lua_reader, static_cast<void *>(&handle), path.c_str())};
  if (lua_laod_stats != 0) {
    glog<log_event::error>("Error while loading Lua script: {}", lua_tostring(L, -1));

    // Step 2.1: free allocated file
    alloc->free(static_cast<void *>((*handle.opt_file_span).data()));

    return false;
  }

  // Step 3: Push the message handler onto the stack
  const auto stack_base{lua_gettop(L)};
  lua_pushcfunction(L, lua_message_handler);
  lua_insert(L, stack_base);

  // Step 4: pCall the lua script
  const auto lua_pcall_stats{lua_pcall(L, 0, LUA_MULTRET, stack_base)};
  lua_remove(L, stack_base);

  if (lua_pcall_stats != 0) {
    glog<log_event::error>("Error while executing Lua script:\n{}", lua_tostring(L, -1));

    // Step 4.1: free allocated file
    alloc->free(static_cast<void *>((*handle.opt_file_span).data()));

    return false;
  }

  // Step 5: free allocated file
  alloc->free(static_cast<void *>((*handle.opt_file_span).data()));

  return true;
}

void surge::vm_colored_print(lua_State *L, fmt::string_view banner,
                             const fmt::text_style &style) noexcept {
  try {
    // clang-format off
      fmt::print(
                #ifdef SURGE_USE_LOG_COLOR
                 style,
                #endif
                "SURGE Lua VM {}: ",
                banner
      );
    // clang-format on

    int nargs = lua_gettop(L);

    for (int i = 1; i <= nargs; ++i) {
      if (lua_isboolean(L, i)) {
        fmt::print("{}", static_cast<bool>(lua_toboolean(L, i)));
      } else if (lua_isnil(L, i)) {
        fmt::print("nill");
      } else {
        fmt::print("{}", lua_tostring(L, i));
      }
    }

    fmt::print("\n");

  } catch (const std::exception &e) {
    std::cout << "Error while invonking fmt: " << e.what() << std::endl;
  }
}