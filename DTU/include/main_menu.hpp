#ifndef SURGE_DTU_STATE_MAIN_MENU
#define SURGE_DTU_STATE_MAIN_MENU

#include "DTU.hpp"
#include "states.hpp"

#include <glm/glm.hpp>

namespace DTU::state::main_menu {

auto load(cmdq_t &cmdq, vec_glui &ids, vec_glui64 &handles, sdl_t &sdl, float ww, float wh) noexcept
    -> int;

void unload(cmdq_t &cmdq, vec_glui &ids, vec_glui64 &handles, sdl_t &sdl) noexcept;

void update(cmdq_t &cmdq, sdl_t &sdl, double dt) noexcept;

void keyboard_event(cmdq_t &cmdq, int key, int scancode, int action, int mods) noexcept;

} // namespace DTU::state::main_menu

#endif // SURGE_DTU_STATE_MAIN_MENU