// clang-format off
#include "new_game/new_game.hpp"
#include "ui/character_sheet.hpp"

#include "player/logging.hpp"
// clang-format on

auto DTU::state::new_game::load(cmdq_t &, vec_glui &ids, vec_glui64 &handles, sdl_t &sdl, float ww,
                                float wh) noexcept -> int {

  log_info("Loading new_game state");

  ui::character_sheet::load(ids, handles, sdl, ww, wh);

  return 0;
}

void DTU::state::new_game::unload(cmdq_t &, sdl_t &) noexcept {
  log_info("Unloading main_menu state");
  return;
}

void DTU::state::new_game::update(GLFWwindow *, cmdq_t &, sdl_t &, tdd_t &, tgl_t &,
                                  double) noexcept {
  // TODO
}

void DTU::state::new_game::mouse_click(cmdq_t &, GLFWwindow *, int, int, int) noexcept {
  // TODO
}

void DTU::state::new_game::mouse_scroll(cmdq_t &, GLFWwindow *, double, double) noexcept {
  // TODO
}
