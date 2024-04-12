#ifndef SURGE_DTU_STATE_MAIN_MENU
#define SURGE_DTU_STATE_MAIN_MENU

// clang-format off
#include "type_aliases.hpp"

#include "player/error_types.hpp"

#include <optional>
// clang-format on

namespace DTU::state_impl::main_menu {

auto load(tdb_t &tdb) noexcept -> std::optional<surge::error>;
auto unload(tdb_t &tdb) noexcept -> std::optional<surge::error>;

auto update(GLFWwindow *window, double dt, tdb_t &tdb, sdb_t &sdb, txd_t &txd) noexcept
    -> std::optional<surge::error>;

} // namespace DTU::state_impl::main_menu

#endif // SURGE_DTU_STATE_MAIN_MENU