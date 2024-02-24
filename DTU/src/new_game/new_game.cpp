// clang-format off
#include "new_game/new_game.hpp"
#include "ui/character_sheet.hpp"

#include "player/logging.hpp"
// clang-format on

auto DTU::state::new_game::load(vec_glui &, vec_glui64 &) noexcept -> int {
  log_info("Loading new_game state");

  // ui::character_sheet::load(ids, handles);

  return 0;
}

void DTU::state::new_game::unload(cmdq_t &, sdl_t &) noexcept {
  log_info("Unloading main_menu state");
  return;
}

void DTU::state::new_game::update(GLFWwindow *, cmdq_t &, const sbd_t &, sdl_t &, DTU::tdd_t &,
                                  DTU::tgl_t &, double) noexcept {

  // const auto [ww, wh] = surge::window::get_dims(window);
  // const auto cursor_pos{surge::window::get_cursor_pos(window)};
  // ui::character_sheet::update(cmdq, ui_sbd, ui_sdl, tdd, tgd, glm::vec2{ww, wh}, cursor_pos);
}

void DTU::state::new_game::mouse_click(cmdq_t &, GLFWwindow *, int, int, int) noexcept {

  // if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
  //   ui::character_sheet::mouse_left_click(cmdq, surge::window::get_cursor_pos(window));
  // }
}

void DTU::state::new_game::mouse_scroll(cmdq_t &, GLFWwindow *, double, double) noexcept {
  // if (yoffset > 0) {
  //   ui::character_sheet::mouse_scroll_up(cmdq, surge::window::get_cursor_pos(window));
  // }

  // if (yoffset < 0) {
  //   ui::character_sheet::mouse_scroll_down(cmdq, surge::window::get_cursor_pos(window));
  // }
}
