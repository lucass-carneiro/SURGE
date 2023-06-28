#include "lua/lua_utils.hpp"

// clang-format off
#include "lua/lua_bindings.hpp"
#include "lua/lua_states.hpp"
#include "lua/lua_wrappers.hpp"
// clang-format on

#include "file.hpp"
#include "options.hpp"
#include "safe_ops.hpp"

#ifdef SURGE_ENABLE_TRACY
#  include <tracy/Tracy.hpp>
#endif

struct lua_file_handle {
  surge::load_file_return_t opt_file_span;
  bool file_read{false};
};

auto lua_reader(lua_State *, void *user_data, std::size_t *chunck_size) noexcept -> const char *;
auto lua_message_handler(lua_State *L) noexcept -> int;

auto surge::do_file_at(lua_State *L, const std::filesystem::path &path) noexcept -> bool {
  // See example implementation from the Lua interpreter here:
  // https://www.lua.org/source/5.4/lua.c.html#msghandler

#ifdef SURGE_SYSTEM_Windows
  log_info(L"Doing Lua script {} at VM {:#x}", path.c_str(), reinterpret_cast<std::uintptr_t>(L));
#else
  log_info("Doing Lua script {} at VM {:#x}", path.c_str(), reinterpret_cast<std::uintptr_t>(L));
#endif

  // Step 1: load file and construct a lua_file_handle object to pass to
  // lua_reader
  lua_file_handle handle{load_file(path, ".lua", false), false};
  if (!handle.opt_file_span) {
    return false;
  }

// Step 2: Load Lua chunk
#ifdef SURGE_SYSTEM_Windows
  const auto lua_laod_stats{
      lua_load(L, &lua_reader, static_cast<void *>(&handle), path.string().c_str())};
#else
  const auto lua_laod_stats{lua_load(L, &lua_reader, static_cast<void *>(&handle), path.c_str())};
#endif

  if (lua_laod_stats != 0) {
    log_error("Error while loading Lua script: {}", lua_tostring(L, -1));

    // Step 2.1: free allocated file
    mi_free(static_cast<void *>((*handle.opt_file_span).data()));

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
    log_error("Error while executing Lua script:\n{}", lua_tostring(L, -1));

    // Step 4.1: free allocated file
    mi_free(static_cast<void *>((*handle.opt_file_span).data()));

    return false;
  }

  // Step 5: free allocated file
  mi_free(static_cast<void *>((*handle.opt_file_span).data()));

  return true;
}

auto lua_reader(lua_State *, void *user_data, std::size_t *chunck_size) noexcept -> const char * {
  auto handle{static_cast<lua_file_handle *>(user_data)};

  if (!handle->file_read) {
    *chunck_size = handle->opt_file_span.value().size();
    handle->file_read = true;
    return static_cast<const char *>(static_cast<void *>(handle->opt_file_span.value().data()));
  } else {
    *chunck_size = 0;
    return nullptr;
  }
}

auto lua_message_handler(lua_State *L) noexcept -> int {
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

auto surge::do_file_at_idx(std::size_t i, const std::filesystem::path &path) noexcept -> bool {
#ifdef SURGE_ENABLE_TRACY
  ZoneScoped;
#endif

  auto &lua_state{surge::global_lua_states::get().at(i)};
  return do_file_at(lua_state.get(), path);
}