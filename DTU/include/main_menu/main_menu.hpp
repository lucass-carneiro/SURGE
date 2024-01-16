#ifndef SURGE_DTU_STATE_MAIN_MENU
#define SURGE_DTU_STATE_MAIN_MENU

#include "error_types.hpp"

#include <optional>

namespace DTU::state::main_menu {

auto state_load() noexcept -> std::optional<DTU::error>;
auto state_unload() noexcept -> std::optional<DTU::error>;

} // namespace DTU::state::main_menu

#endif // SURGE_DTU_STATE_MAIN_MENU