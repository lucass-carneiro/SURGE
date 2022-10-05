#ifndef SURGE_LUAVM_HPP
#define SURGE_LUAVM_HPP

#include "allocators/global_allocators.hpp"
#include "allocators/stl_allocator.hpp"

// clang-format off
#include <lua.hpp>
// clang-format on

#include <array>
#include <cstddef>
#include <optional>
#include <type_traits>

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

/**
 * @brief Pushes the engine configuration table in the specified VM of the global_lua_states array
 *
 * @param i The index in the global_lua_states array to push to.
 */
void push_engine_config_at(std::size_t i) noexcept;

struct lua_engine_config {
  lua_Integer window_width{0};
  lua_Integer window_height{0};
  lua_String window_name{nullptr};
  lua_Boolean windowed{false};
  lua_Integer window_monitor_index{0};
  std::array<lua_Number, 4> clear_color{};
};

auto get_lua_engine_config(lua_State *L) noexcept -> std::optional<lua_engine_config>;

auto do_file_at(std::size_t i, const std::filesystem::path &path) noexcept -> bool;

auto lua_reader(lua_State *L, void *user_data, std::size_t *chunck_size) noexcept -> const char *;

auto lua_message_handler(lua_State *L) noexcept -> int;

/**
 * @brief The type of event to be logged in the resident VM
 *
 */
enum class vm_log_event : std::uint8_t { warning, error, message, memory, count };

/**
 * @brief Type of a static hash map of events and colors for the resident VMs.
 *
 */
using vm_log_color_map_t
    = static_map<vm_log_event, fmt::text_style, static_cast<std::size_t>(vm_log_event::count)>;

/**
 * @brief Type of a static hash map of events and banners for the resident VMs.
 *
 */
using vm_log_banner_map_t
    = static_map<vm_log_event, const char *, static_cast<std::size_t>(vm_log_event::count)>;

/**
 * @brief Event-color hash map for the resident VMs
 *
 */
constexpr const vm_log_color_map_t vm_log_color_map{
    {{{vm_log_event::warning, fmt::fg(fmt::color::wheat)},
      {vm_log_event::error, fmt::emphasis::bold | fg(fmt::color::indian_red)},
      {vm_log_event::message, fg(fmt::color::dark_slate_blue)},
      {vm_log_event::memory, fg(fmt::color::cadet_blue)}}}};

/**
 * @brief Event-banner hash map for the resident VMs
 *
 */
constexpr const vm_log_banner_map_t vm_log_banner_map{{{{vm_log_event::warning, "warning"},
                                                        {vm_log_event::error, "error"},
                                                        {vm_log_event::message, "message"},
                                                        {vm_log_event::memory, "memory event"}}}};

/**
 * @brief Prints a string with a colored banner in the VM
 *
 * @param file The file to print to.
 * @param banner The banner of the log message.
 * @param style The style of the log message.
 * @param str The message content
 * @param args Arguments to print in the message.
 */
void vm_colored_print(lua_State *L, fmt::string_view banner, const fmt::text_style &style) noexcept;

template <vm_log_event e> inline void vm_log(lua_State *L) noexcept {
  vm_colored_print(L, vm_log_banner_map[e], vm_log_color_map[e]);
}

inline auto lua_log_message(lua_State *L) noexcept -> int {
  vm_log<vm_log_event::message>(L);
  return 0;
}

inline auto lua_log_warning(lua_State *L) noexcept -> int {
  vm_log<vm_log_event::warning>(L);
  return 0;
}

inline auto lua_log_error(lua_State *L) noexcept -> int {
  vm_log<vm_log_event::error>(L);
  return 0;
}

inline auto lua_log_memory(lua_State *L) noexcept -> int {
  vm_log<vm_log_event::memory>(L);
  return 0;
}

} // namespace surge

#endif // SURGE_LUAVM_HPP