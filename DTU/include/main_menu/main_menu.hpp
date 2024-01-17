#ifndef SURGE_DTU_STATE_MAIN_MENU
#define SURGE_DTU_STATE_MAIN_MENU

#include "game_state.hpp"

namespace DTU::state::main_menu {

auto state_load() noexcept -> int;
auto state_unload() noexcept -> int;
auto draw() noexcept -> int;

} // namespace DTU::state::main_menu

#endif // SURGE_DTU_STATE_MAIN_MENU