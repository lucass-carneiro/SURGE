#ifndef SURGE_LUA_BINDINGS_HPP
#define SURGE_LUA_BINDINGS_HPP

#include "lua_states.hpp"

#include <type_traits>

namespace surge {

using lua_String = const char *;
using lua_Boolean = bool;

/**
 * @brief Pushes a scalar value in the Lua stack
 *
 * @tparam T The type to push
 * @param L The lua state
 * @param value The value to push
 */
template <typename T> inline void push_scalar(lua_State *L, T value) noexcept {
  if constexpr (std::is_same<T, lua_Integer>::value) {
    lua_pushinteger(L, value);
  } else if constexpr (std::is_same<T, lua_Number>::value) {
    lua_pushnumber(L, value);
  } else if constexpr (std::is_same<T, lua_String>::value) {
    lua_pushstring(L, value);
  } else if constexpr (std::is_same<T, void>::value) {
    lua_pushnil(L);
  } else if constexpr (std::is_same<T, bool>::value) {
    lua_pushboolean(L, static_cast<int>(value));
  } else if constexpr (std::is_same<T, lua_CFunction>::value) {
    lua_pushcfunction(L, value);
  }
}

/**
 * @brief Pushes a generic type in the lua stack
 *
 * @tparam T The type to push
 * @param L The lua state
 * @param value The value to push
 */
template <typename T> inline void push(lua_State *L, T value) noexcept { push_scalar(L, value); }

/**
 * @brief Pushes a generic value to a specific lua stack in the global_lua_states array
 *
 * @tparam T The type to push
 * @param i The index of the lua state to push to
 * @param value The value to push
 */
template <typename T> inline void push(std::size_t i, T value) noexcept {
  auto L = global_lua_states::get().at(i).get();
  push(L, value);
}

/**
 * @brief Pushes a table field in the lua stack. Assumes that a table was previously declared with
 * lua_newtable(L);
 *
 * @tparam key_t The type of the table key
 * @tparam value_t The type of the value
 * @param L The lua state
 * @param key The key
 * @param value The value
 */
template <typename key_t, typename value_t>
inline void add_table_field(lua_State *L, key_t key, value_t value) noexcept {
  push<key_t>(L, key);
  push<value_t>(L, value);
  lua_settable(L, -3);
}

template <typename element_t, std::size_t array_size>
inline void push_array(lua_State *L, std::array<element_t, array_size> &&array) {
  for (lua_Integer i = 1; const auto &element : array) {
    add_table_field<lua_Integer, element_t>(L, i, element);
    i++;
  }
}

} // namespace surge

#endif // SURGE_LUA_BINDINGS_HPP