#ifndef SURGE_GLOBAL_LUA_STATES_HPP
#define SURGE_GLOBAL_LUA_STATES_HPP

#include "allocators/global_allocators.hpp"
#include "allocators/stl_allocator.hpp"

// clang-format off
#include <lua.hpp>
// clang-format on

#include <memory>
#include <vector>

namespace surge {

/**
 * @brief Global array of lua state pointers
 *
 */
class global_lua_states {
public:
  using lua_state_ptr = std::unique_ptr<lua_State, void (*)(lua_State *)>;
  using stl_allocator_t = stl_allocator<lua_state_ptr, linear_arena_allocator>;
  using state_vec_t = std::vector<lua_state_ptr, stl_allocator_t>;

  static inline auto get() -> global_lua_states & {
    static global_lua_states states;
    return states;
  }

  void init() noexcept;

  [[nodiscard]] auto at(std::size_t i) noexcept -> lua_state_ptr &;
  [[nodiscard]] auto back() noexcept -> lua_state_ptr &;

  ~global_lua_states() noexcept = default;

  global_lua_states(const global_lua_states &) = delete;
  global_lua_states(global_lua_states &&) = delete;

  auto operator=(global_lua_states) -> global_lua_states & = delete;

  auto operator=(const global_lua_states &) -> global_lua_states & = delete;

  auto operator=(global_lua_states &&) -> global_lua_states & = delete;

private:
  global_lua_states() = default;

  linear_arena_allocator *parent_allocator{&global_linear_arena_allocator::get()};
  stl_allocator_t parent_stl_allocator{parent_allocator};

  state_vec_t state_array{parent_stl_allocator};
};

} // namespace surge

#endif // SURGE_GLOBAL_LUA_STATES_HPP