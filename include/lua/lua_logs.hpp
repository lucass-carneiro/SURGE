#ifndef SURGE_VM_LOGS_HPP
#define SURGE_VM_LOGS_HPP

#include "static_map.hpp"

// clang-format off
#include <luajit/lua.hpp>
// clang-format on

#include <fmt/color.h>

namespace surge {

/**
 * @brief The type of event to be logged in the resident VM
 *
 */
enum class vm_log_event : short { warning, error, message, memory, count };

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

#endif // SURGE_VM_LOGS_HPP