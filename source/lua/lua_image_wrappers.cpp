#include "allocator.hpp"
#include "entities/image.hpp"
#include "log.hpp"
#include "lua/lua_wrappers.hpp"

auto surge::lua_new_image(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 7) {
    log_warn("Function new expected 7 arguments and instead got {} "
             "arguments. Returning nil",
             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!lua_isstring(L, 1)) {
    log_warn("Fucntion new expects arguents 1 to be a string. Returning nil");
    lua_pushnil(L);
    return 1;
  }

  for (int i = 2; i <= 7; i++) {
    if (!lua_isnumber(L, i)) {
      log_warn("Function new expected argument {} to be a number. Returning nil", i);
      lua_pushnil(L);
      return 1;
    }
  }

  // Argument extraction
  const char *sheet_path_str{lua_tostring(L, 1)};

  const auto pos_x{static_cast<float>(lua_tonumber(L, 2))};
  const auto pos_y{static_cast<float>(lua_tonumber(L, 3))};
  const auto pos_z{static_cast<float>(lua_tonumber(L, 4))};

  const auto scale_x{static_cast<float>(lua_tonumber(L, 5))};
  const auto scale_y{static_cast<float>(lua_tonumber(L, 6))};
  const auto scale_z{static_cast<float>(lua_tonumber(L, 7))};

  // Internal call
  auto image_buffer{mi_malloc(sizeof(image_entity))};
  image_entity *img_ptr{new (image_buffer) image_entity(
      sheet_path_str, glm::vec3{pos_x, pos_y, pos_z}, glm::vec3{scale_x, scale_y, scale_z})};

  // Pass this pointer to the Lua VM as userdata
  auto vm_background_ptr{static_cast<image_entity **>(lua_newuserdata(L, sizeof(void *)))};
  *vm_background_ptr = img_ptr;

  lua_getglobal(L, "surge");
  lua_getfield(L, -1, "image");
  lua_getfield(L, -1, "image_meta");
  lua_setmetatable(L, -4);
  lua_pop(L, 2);

  return 1;
}

auto surge::lua_drop_image(lua_State *L) noexcept -> int {
  // Data recovery
  auto vm_img_ptr{static_cast<image_entity **>(lua_touserdata(L, 1))};
  auto img_ptr{*vm_img_ptr};

  // Data cleanup
  mi_free(img_ptr);

  return 0;
}

[[nodiscard]] static auto is_image(lua_State *L, const char *func_name) noexcept -> bool {
  if (!lua_isuserdata(L, 1)) {
    surge::log_warn("Function {} expected 1 user data argument. Returning nil", func_name);
    return false;
  }

  if (!lua_getmetatable(L, 1)) {
    surge::log_warn("User data does not have a metatable");
    return false;
  }

  lua_getfield(L, -1, "__name");
  const auto name{lua_tostring(L, -1)};

  if (std::strcmp(name, "surge::image") != 0) {
    surge::log_warn("Expected surge::image userdata and recieved {}", name);
    return false;
  }

  lua_pop(L, 2);
  return true;
}

auto surge::lua_draw_image(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 1) {
    log_warn("Function draw expected 1 user data arguments and instead got "
             "{} arguments. Returning nil",
             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!is_image(L, "draw")) {
    lua_pushnil(L);
    return 1;
  }

  // Data recovery
  auto vm_image_ptr{static_cast<image_entity **>(lua_touserdata(L, 1))};
  auto img_ptr{*vm_image_ptr};

  // Internal call
  img_ptr->draw();

  lua_pop(L, 1);

  return 0;
}

auto surge::lua_image_toggle_h_flip(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 1) {
    log_warn("Function toggle_h_flip expected 1 user data arguments and instead got "
             "{} arguments. Returning nil",
             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!is_image(L, "toggle_h_flip")) {
    lua_pushnil(L);
    return 1;
  }

  // Data recovery
  auto vm_image_ptr{static_cast<image_entity **>(lua_touserdata(L, 1))};
  auto img_ptr{*vm_image_ptr};

  // Internal call
  img_ptr->toggle_h_flip();

  lua_pop(L, 1);

  return 0;
}

auto surge::lua_image_toggle_v_flip(lua_State *L) noexcept -> int {
  const auto nargs{lua_gettop(L)};

  // Argument count and type validation
  if (nargs != 1) {
    log_warn("Function toggle_v_flip expected 1 user data arguments and instead got "
             "{} arguments. Returning nil",
             nargs);
    lua_pushnil(L);
    return 1;
  }

  if (!is_image(L, "toggle_v_flip")) {
    lua_pushnil(L);
    return 1;
  }

  // Data recovery
  auto vm_image_ptr{static_cast<image_entity **>(lua_touserdata(L, 1))};
  auto img_ptr{*vm_image_ptr};

  // Internal call
  img_ptr->toggle_v_flip();

  lua_pop(L, 1);

  return 0;
}