#ifndef SURGE_DTU_STATE_MAIN_MENU
#define SURGE_DTU_STATE_MAIN_MENU

#include <glm/glm.hpp>

namespace DTU::state::main_menu {

auto load(float ww, float wh) noexcept -> int;
auto unload() noexcept -> int;
auto draw(unsigned int nuts, glm::mat4 &proj, glm::mat4 &view) noexcept -> int;
auto update(double dt) noexcept -> int;

} // namespace DTU::state::main_menu

#endif // SURGE_DTU_STATE_MAIN_MENU