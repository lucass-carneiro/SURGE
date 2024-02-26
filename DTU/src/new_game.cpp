// clang-format off
#include "new_game.hpp"
#include "character_sheet.hpp"

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

void DTU::state::new_game::update(GLFWwindow *window, cmdq_t &, const sbd_t &ui_sbd, sdl_t &ui_sdl,
                                  const DTU::tbd_t &tbd, DTU::tdd_t &tdd, DTU::tgd_t &tgd,
                                  double) noexcept {

  DTU::clear_sprites(ui_sdl);
  DTU::clear_text(tdd);
  ui::character_sheet::update(window, ui_sdl, tdd, tgd);
  surge::atom::sprite::send_buffers(ui_sbd, ui_sdl);
  surge::atom::text::send_buffers(tbd, tdd);

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
