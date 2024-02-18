// clang-format off
#include "new_game/new_game.hpp"
#include "ui/character_sheet.hpp"

#include "player/logging.hpp"
// clang-format on

auto DTU::state::new_game::load(vec_glui &ids, vec_glui64 &handles) noexcept -> int {

  log_info("Loading new_game state");

  ui::character_sheet::load(ids, handles);

  return 0;
}

void DTU::state::new_game::unload(cmdq_t &, sdl_t &) noexcept {
  log_info("Unloading main_menu state");
  return;
}

void DTU::state::new_game::update(GLFWwindow *window, cmdq_t &cmdq, const sbd_t &ui_sbd,
                                  sdl_t &ui_sdl, DTU::tdd_t &tdd, DTU::tgl_t &tgd,
                                  double) noexcept {

  const auto [ww, wh] = surge::window::get_dims(window);

  ui::character_sheet::update(cmdq, ui_sbd, ui_sdl, tdd, tgd, ww, wh);
}

void DTU::state::new_game::mouse_click(cmdq_t &, GLFWwindow *, int, int, int) noexcept {
  // TODO
}

void DTU::state::new_game::mouse_scroll(cmdq_t &, GLFWwindow *, double, double) noexcept {
  // TODO
}
