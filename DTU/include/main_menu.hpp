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

auto update(GLFWwindow *window, double dt, tdb_t &tdb, sdb_t &sdb) noexcept
    -> std::optional<surge::error>;

/*void update(cmdq_t &cmdq, sdl_t &sdl, double dt) noexcept;

void keyboard_event(cmdq_t &cmdq, int key, int scancode, int action, int mods) noexcept;*/

} // namespace DTU::state_impl::main_menu

#endif // SURGE_DTU_STATE_MAIN_MENU