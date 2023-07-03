#ifndef SURGE_LUA_UTILS_HPP
#define SURGE_LUA_UTILS_HPP

#include "lua_bindings.hpp"

#include <array>
#include <filesystem>
#include <optional>

namespace surge {

struct lua_engine_config {
  lua_Integer window_width{800};
  lua_Integer window_height{600};
  lua_String window_name{"Default Window Name"};
  bool windowed{true};
  lua_Integer window_monitor_index{0};
  bool show_cursor{true};
  lua_Integer show_debug_objects{0};
  std::array<lua_Number, 4> clear_color{0, 0, 0, 1};
  const char *root_dir{"/home/surge/"};
};

void lua_add_engine_context(lua_State *L, std::size_t i) noexcept;

auto lua_get_engine_config(lua_State *L) noexcept -> std::optional<lua_engine_config>;

auto do_file_at(lua_State *L, const char *path) noexcept -> bool;
auto do_file_at_idx(std::size_t i, const char *path) noexcept -> bool;

} // namespace surge

#endif // SURGE_LUA_UTILS_HPP