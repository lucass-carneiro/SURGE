#ifndef SURGE_LUA_UTILS_HPP
#define SURGE_LUA_UTILS_HPP

#include "lua_bindings.hpp"

#include <array>
#include <filesystem>
#include <optional>

namespace surge {

/**
 * @brief Pushes the engine configuration table in the specified VM of the global_lua_states array
 *
 * @param i The index in the global_lua_states array to push to.
 */
void push_engine_config_at(std::size_t i) noexcept;

struct lua_engine_config {
  lua_Integer window_width{800};
  lua_Integer window_height{600};
  lua_String window_name{"Default Window Name"};
  lua_Boolean windowed{true};
  lua_Integer window_monitor_index{0};
  std::array<lua_Number, 4> clear_color{0, 0, 0, 1};
};

auto get_lua_engine_config(lua_State *L) noexcept -> std::optional<lua_engine_config>;

auto do_file_at(std::size_t i, const std::filesystem::path &path) noexcept -> bool;

auto lua_reader(lua_State *L, void *user_data, std::size_t *chunck_size) noexcept -> const char *;

auto lua_message_handler(lua_State *L) noexcept -> int;

} // namespace surge

#endif // SURGE_LUA_UTILS_HPP