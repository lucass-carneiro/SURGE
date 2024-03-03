#ifndef SURGE_DTU_STATE_NEW_GAME
#define SURGE_DTU_STATE_NEW_GAME

// clang-format off
#include "DTU.hpp"

#include <glm/glm.hpp>
// clang-format on

namespace DTU::state::new_game {

auto load(vec_glui &ids, vec_glui64 &handles) noexcept -> int;
void unload(vec_glui &ids, vec_glui64 &handles, sdl_t &ui_sdl, DTU::tdd_t &tdd) noexcept;

void update(GLFWwindow *window, cmdq_t &cmdq, const sbd_t &ui_sbd, sdl_t &ui_sdl,
            const DTU::tbd_t &tbd, DTU::tdd_t &tdd, DTU::tgd_t &tgd, double dt) noexcept;

} // namespace DTU::state::new_game

#endif // SURGE_DTU_STATE_NEW_GAME
