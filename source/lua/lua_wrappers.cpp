#include "lua/lua_wrappers.hpp"

#include "image_loader.hpp"
#include "log.hpp"
#include "opengl/create_program.hpp"
#include "thread_allocators.hpp"

auto surge::lua_load_image(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 2) {
    glog<log_event::warning>(
        "Function load_image expected 2 arguments and instead got {} argument. Returning nil",
        nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!(lua_isstring(L, 1) || lua_isstring(L, 2))) {
    glog<log_event::warning>("Fucntion load_image expects 2 string arguments. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  // Argument extraction
  const char *path_str{lua_tostring(L, 1)};
  const char *ext_str{lua_tostring(L, 2)};

  // VM index recovery
  lua_getglobal(L, "surge");
  lua_getfield(L, -1, "vm_index");
  const auto vm_index{static_cast<std::size_t>(lua_tointeger(L, -1))};
  lua_pop(L, 2);

  // Internal function call
  void *img{load_image(global_thread_allocators::get().at(vm_index).get(), path_str, ext_str)};

  if (img == nullptr) {
    lua_pushnil(L);
  } else {
    lua_pushlightuserdata(L, img);
  }

  return 1;
}

auto surge::lua_drop_image(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 1) {
    glog<log_event::warning>("Function drop_image expected 1 arguments and instead got {} argument",
                             nargs);
    return 0;
  }

  if (!lua_islightuserdata(L, 1)) {
    glog<log_event::warning>("Fucntion load_image expects 1 light user data argument.");
    return 0;
  }

  // Argument extraction
  void *img{lua_touserdata(L, 1)};

  // VM index recovery
  lua_getglobal(L, "surge");
  lua_getfield(L, -1, "vm_index");
  const auto vm_index{static_cast<std::size_t>(lua_tointeger(L, -1))};
  lua_pop(L, 2);

  stbi_image_free(global_thread_allocators::get().at(vm_index).get(), img);
  lua_pushnil(L);

  return 1;
}

auto surge::lua_create_program(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 2) {
    glog<log_event::warning>(
        "Function ceate_program expected 2 arguments and instead got {} argument. Returning nil",
        nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!(lua_isstring(L, 1) || lua_isstring(L, 2))) {
    glog<log_event::warning>("Fucntion ceate_program expects 2 string arguments. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  // Argument extraction
  const char *vertex_shader_path{lua_tostring(L, 1)};
  const char *fragment_shader_path{lua_tostring(L, 2)};

  // VM index recovery
  lua_getglobal(L, "surge");
  lua_getfield(L, -1, "vm_index");
  const auto vm_index{static_cast<std::size_t>(lua_tointeger(L, -1))};
  lua_pop(L, 2);

  // Internal function call
  const auto program_handle{create_program(global_thread_allocators::get().at(vm_index).get(),
                                           vertex_shader_path, fragment_shader_path)};

  if (!program_handle) {
    lua_pushnil(L);
  } else {
    lua_pushinteger(L, static_cast<lua_Integer>(*program_handle));
  }

  return 1;
}

auto surge::lua_get_shader_program_idx(lua_State *L) noexcept -> std::optional<lua_Integer> {
  lua_getglobal(L, "surge");
  lua_getfield(L, -1, "current_shader_program");

  if (!lua_isnumber(L, -1)) {
    lua_pop(L, 2);
    return {};
  } else {
    const auto shader_program{lua_tointeger(L, -1)};
    lua_pop(L, 2);
    return shader_program;
  }
}

auto surge::lua_load_callback(lua_State *L) noexcept -> bool {
  lua_getglobal(L, "surge");
  lua_getfield(L, -1, "load");

  const auto pcall_result{lua_pcall(L, 0, 0, 0)};

  if (pcall_result != 0) {
    glog<log_event::error>("Unable to call surge.load: {}", lua_tostring(L, -1));
    lua_pop(L, 3);
    return false;
  } else {
    lua_pop(L, 2);
    return true;
  }
}