#ifndef SURGE_DTU_STATE_NEW_GAME
#define SURGE_DTU_STATE_NEW_GAME

// clang-format off
#include "DTU.hpp"
#include "commands.hpp"

#include <glm/glm.hpp>
// clang-format on

namespace DTU::state::new_game {

auto load(vec_glui &ids, vec_glui64 &handles) noexcept -> int;
void unload(cmdq_t &cmdq, sdl_t &dl) noexcept;

void update(GLFWwindow *window, cmdq_t &cmdq, const sbd_t &ui_sbd, sdl_t &ui_sdl, DTU::tdd_t &tdd,
            DTU::tgl_t &tgd, double dt) noexcept;

void mouse_click(cmdq_t &cmdq, GLFWwindow *window, int button, int action, int mods) noexcept;
void mouse_scroll(cmdq_t &cmdq, GLFWwindow *window, double xoffset, double yoffset) noexcept;

} // namespace DTU::state::new_game

#endif // SURGE_DTU_STATE_NEW_GAME
