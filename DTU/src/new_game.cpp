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

void DTU::state::new_game::unload(vec_glui &ids, vec_glui64 &handles, sdl_t &ui_sdl,
                                  DTU::tdd_t &tdd) noexcept {
  log_info("Unloading main_menu state");
  DTU::unload_textures(ids, handles);
  DTU::clear_sprites(ui_sdl);
  DTU::clear_text(tdd);
}

void DTU::state::new_game::update(GLFWwindow *window, cmdq_t &, const sbd_t &ui_sbd, sdl_t &ui_sdl,
                                  const DTU::tbd_t &tbd, DTU::tdd_t &tdd, DTU::tgd_t &tgd,
                                  double) noexcept {

  DTU::clear_sprites(ui_sdl);
  DTU::clear_text(tdd);

  ui::character_sheet::update(window, ui_sdl, tdd, tgd);

  surge::atom::sprite::send_buffers(ui_sbd, ui_sdl);
  surge::atom::text::send_buffers(tbd, tdd);
}
