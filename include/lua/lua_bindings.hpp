#ifndef SURGE_LUA_BINDINGS_HPP
#define SURGE_LUA_BINDINGS_HPP

#include "logging_system/logging_system.hpp"
#include "lua_states.hpp"

#include <optional>
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
template <typename T> inline void lua_push_scalar(lua_State *L, T value) noexcept {
  if constexpr (std::is_same<T, lua_Integer>::value) {
    lua_pushinteger(L, value);
  } else if constexpr (std::is_same<T, lua_Number>::value) {
    lua_pushnumber(L, value);
  } else if constexpr (std::is_same<T, const char *>::value) {
    lua_pushstring(L, value);
  } else if constexpr (std::is_same<T, void>::value) {
    lua_pushnil(L);
  } else if constexpr (std::is_same<T, bool>::value) {
    lua_pushboolean(L, static_cast<int>(value));
  } else if constexpr (std::is_same<T, lua_CFunction>::value) {
    lua_pushcfunction(L, value);
  } else if constexpr (std::is_same<T, void *>::value) {
    lua_pushlightuserdata(L, value);
  }
}

/**
 * @brief Pushes a generic type in the lua stack
 *
 * @tparam T The type to push
 * @param L The lua state
 * @param value The value to push
 */
template <typename T> inline void lua_push(lua_State *L, T value) noexcept {
  lua_push_scalar(L, value);
}

/**
 * @brief Pushes a generic value to a specific lua stack in the global_lua_states array
 *
 * @tparam T The type to push
 * @param i The index of the lua state to push to
 * @param value The value to push
 */
template <typename T> inline void lua_push(std::size_t i, T value) noexcept {
  auto L = lua_states::at(i).get();
  lua_push(L, value);
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
inline void lua_add_table_field(lua_State *L, key_t key, value_t value) noexcept {
  lua_push<key_t>(L, key);
  lua_push<value_t>(L, value);
  lua_settable(L, -3);
}

/**
 * @brief Pushes an array to the lua stack
 *
 * @tparam element_t Type of the element array
 * @tparam array_size Size of the array
 * @param L The lua state
 * @param array The array
 */
template <typename element_t, std::size_t array_size>
inline void lua_push_array(lua_State *L, std::array<element_t, array_size> &&array) {
  for (lua_Integer i = 1; const auto &element : array) {
    lua_add_table_field<lua_Integer, element_t>(L, i, element);
    i++;
  }
}

template <typename T> [[nodiscard]] inline auto lua_get_field(lua_State *L, const char *root_table,
                                                              const char *field_name) noexcept
    -> std::optional<T> {

  // Get global root table
  lua_getglobal(L, root_table);

  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);
    log_error("Global table {} not found.", root_table);
    return {};
  }

  // Get field from table
  lua_getfield(L, -1, field_name);

  if constexpr (std::is_same<T, lua_Integer>::value) {
    if (!lua_isnumber(L, -1)) {
      lua_pop(L, 2);
      log_error("integer field {} not found", field_name);
      return {};
    } else {
      const T value{lua_tointeger(L, -1)};
      lua_pop(L, 2);
      return value;
    }
  } else if constexpr (std::is_same<T, lua_Number>::value) {
    if (!lua_isnumber(L, -1)) {
      lua_pop(L, 2);
      log_error("numeric field {} not found", field_name);
      return {};
    } else {
      const T value{lua_tonumber(L, -1)};
      lua_pop(L, 2);
      return value;
    }
  } else if constexpr (std::is_same<T, lua_Boolean>::value) {
    if (!lua_isboolean(L, -1)) {
      lua_pop(L, 2);
      log_error("boolean field {} not found", field_name);
      return {};
    } else {
      const T value{static_cast<bool>(lua_toboolean(L, -1))};
      lua_pop(L, 2);
      return value;
    }
  } else if constexpr (std::is_same<T, const char *>::value) {
    if (!lua_isstring(L, -1)) {
      lua_pop(L, 2);
      log_error("path string field {} not found", field_name);
      return {};
    } else {
      const T value{lua_tostring(L, -1)};
      lua_pop(L, 2);
      return value;
    }
  } else {
    lua_pop(L, 2);
    log_warn("Unable to read {} because it's type is not implemented", field_name);
    return {};
  }
}

} // namespace surge

#endif // SURGE_LUA_BINDINGS_HPP