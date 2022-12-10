#ifndef SURGE_LUA_WRAPPERS_HPP
#define SURGE_LUA_WRAPPERS_HPP

#include <filesystem>
#include <glm/mat4x4.hpp>
#include <lua.hpp>
#include <optional>

namespace surge {

auto lua_load_image(lua_State *L) noexcept -> int;
auto lua_drop_image(lua_State *L) noexcept -> int;

auto lua_load_sprite(lua_State *L) noexcept -> int;
auto lua_drop_sprite(lua_State *L) noexcept -> int;
auto lua_draw_sprite(lua_State *L) noexcept -> int;
auto lua_scale_sprite(lua_State *L) noexcept -> int;

auto lua_create_program(lua_State *L) noexcept -> int;
auto lua_get_shader_program_idx(lua_State *L) noexcept -> std::optional<lua_Integer>;

auto lua_get_current_projection_matrix(lua_State *L, float window_width,
                                       float window_height) noexcept -> glm::mat4;

auto lua_pre_loop_callback(lua_State *L) noexcept -> bool;
void lua_draw_callback(lua_State *L) noexcept;

} // namespace surge

#endif // SURGE_LUA_WRAPPERS_HPP