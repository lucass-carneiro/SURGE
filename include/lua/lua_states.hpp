#ifndef SURGE_GLOBAL_LUA_STATES_HPP
#define SURGE_GLOBAL_LUA_STATES_HPP

#include "allocator.hpp"

// clang-format off
#include <luajit/lua.hpp>
#include <EASTL/vector.h>
// clang-format on

#include <memory>

namespace surge::lua_states {

using lua_state_ptr = std::unique_ptr<lua_State, void (*)(lua_State *)>;
using stl_allocator_t = mi_stl_allocator<lua_state_ptr>;
using state_vec_t = eastl::vector<lua_state_ptr, eastl_allocator>;

extern state_vec_t state_array;

auto init() noexcept -> bool;
auto configure(const char *path) noexcept -> bool;
auto at(std::size_t i) noexcept -> lua_state_ptr &;

} // namespace surge::lua_states

#endif // SURGE_GLOBAL_LUA_STATES_HPP