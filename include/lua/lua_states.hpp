#ifndef SURGE_GLOBAL_LUA_STATES_HPP
#define SURGE_GLOBAL_LUA_STATES_HPP

#include "allocator.hpp"

// clang-format off
#include <luajit/lua.hpp>
#include <EASTL/vector.h>
// clang-format on

#include <filesystem>
#include <memory>

namespace surge {

/**
 * @brief Global array of lua state pointers
 *
 */
class global_lua_states {
public:
  using lua_state_ptr = std::unique_ptr<lua_State, void (*)(lua_State *)>;
  using stl_allocator_t = mi_stl_allocator<lua_state_ptr>;
  using state_vec_t = eastl::vector<lua_state_ptr, eastl_allocator>;

  static inline auto get() -> global_lua_states & {
    static global_lua_states states;
    return states;
  }

  auto init() noexcept -> bool;
  auto configure(const char *path) noexcept -> bool;

  [[nodiscard]] auto at(std::size_t i) noexcept -> lua_state_ptr &;
  [[nodiscard]] auto back() noexcept -> lua_state_ptr &;
  [[nodiscard]] inline auto get_state_array() const noexcept -> const state_vec_t & {
    return state_array;
  }
  [[nodiscard]] auto size() const noexcept -> std::size_t { return state_array.size(); }

  ~global_lua_states() noexcept;

  global_lua_states(const global_lua_states &) = delete;
  global_lua_states(global_lua_states &&) = delete;

  auto operator=(global_lua_states) -> global_lua_states & = delete;

  auto operator=(const global_lua_states &) -> global_lua_states & = delete;

  auto operator=(global_lua_states &&) -> global_lua_states & = delete;

private:
  global_lua_states() = default;

  state_vec_t state_array{eastl_allocator()};
};

} // namespace surge

#endif // SURGE_GLOBAL_LUA_STATES_HPP