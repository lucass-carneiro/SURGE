#include "lua/lua_wrappers.hpp"

#include "image_loader.hpp"
#include "log.hpp"
#include "lua/lua_bindings.hpp"
#include "opengl/create_program.hpp"
#include "thread_allocators.hpp"

#include <glm/gtc/matrix_transform.hpp>

auto surge::lua_load_image(lua_State *L) noexcept -> int {
  glog<log_event::message>("lua_load_image called");

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
  auto img_optional{
      load_image(global_thread_allocators::get().at(vm_index).get(), path_str, ext_str)};

  if (!img_optional) {
    lua_pushnil(L);
  } else {
    // Create and image object on the engine heap and assing the data of the optional to it
    auto img_ptr{
        static_cast<image *>(global_thread_allocators::get().at(vm_index)->malloc(sizeof(image)))};
    *img_ptr = img_optional.value();

    // Pass this pointer to the Lua VM as userdata
    auto vm_img_ptr{static_cast<image **>(lua_newuserdata(L, sizeof(void *)))};
    *vm_img_ptr = img_ptr;

    lua_getglobal(L, "surge");
    lua_getfield(L, -1, "image_meta");
    lua_setmetatable(L, -3);
    lua_pop(L, 1);
  }

  return 1;
}

auto surge::lua_drop_image(lua_State *L) noexcept -> int {
  glog<log_event::message>("lua_drop_image called");

  // Argument extraction
  auto vm_img_ptr{static_cast<image **>(lua_touserdata(L, 1))};
  auto img_ptr{*vm_img_ptr};

  // VM index recovery
  lua_getglobal(L, "surge");
  lua_getfield(L, -1, "vm_index");
  const auto vm_index{static_cast<std::size_t>(lua_tointeger(L, -1))};
  lua_pop(L, 2);

  // Data cleanup
  stbi_image_free(global_thread_allocators::get().at(vm_index).get(), img_ptr->data);
  global_thread_allocators::get().at(vm_index)->free(img_ptr);

  return 0;
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

auto surge::lua_pre_loop_callback(lua_State *L) noexcept -> bool {
  lua_getglobal(L, "surge");
  lua_getfield(L, -1, "pre_loop");

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

auto surge::lua_get_current_projection_matrix(lua_State *L, float window_width,
                                              float window_height) noexcept -> glm::mat4 {

  const auto z_near{lua_get_field<lua_Number>(L, "surge", "z_near")};
  const auto z_far{lua_get_field<lua_Number>(L, "surge", "z_far")};
  const auto perspective_projection{
      lua_get_field<lua_Boolean>(L, "surge", "perspective_projection")};

  if (!z_near) {
    glog<log_event::warning>("Unable to read surge.z_near. Using the default -1.0");
  }

  if (!z_far) {
    glog<log_event::warning>("Unable to read surge.z_far. Using the default -1.0");
  }

  if (!perspective_projection) {
    glog<log_event::warning>(
        "Unable to read surge.perspective_projection. Using the default false");
  }

  if (perspective_projection.value_or(false)) {
    const auto field_of_view{lua_get_field<lua_Number>(L, "surge", "field_of_view")};

    if (!perspective_projection) {
      glog<log_event::warning>("Unable to read surge.field_of_view. Using the default 45 degrees");
    }

    return glm::perspective(glm::degrees(static_cast<float>(field_of_view.value_or(45.0))),
                            window_width / window_height, static_cast<float>(z_near.value_or(-1.0)),
                            static_cast<float>(z_far.value_or(1.0)));
  } else {
    return glm::ortho(0.0f, window_width, window_height, 0.0f,
                      static_cast<float>(z_near.value_or(-1.0)),
                      static_cast<float>(z_far.value_or(1.0)));
  }
}