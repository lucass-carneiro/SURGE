#include "ui/character_sheet.hpp"

#include <cmath>

void DTU::ui::character_sheet::update(GLFWwindow *window, cmdq_t &cmdq,
                                      ui_state_desc &current_state) noexcept {
  using std::abs;

  // Handle window resizes
  const auto [ww, wh] = surge::window::get_dims(window);
  const auto delta_w{abs(current_state.background.scale[0] - ww)};
  const auto delta_h{abs(current_state.background.scale[1] - wh)};

  if (delta_w > 1.0f || delta_h > 1.0f) {
    current_state.background.scale[0] = ww;
    current_state.background.scale[1] = wh;
    current_state.background.scale[2] = 1.0f;
    cmdq.push_back(commands::ui_refresh);
    return;
  }

  // Handle UI commands
  switch (cmdq.size() == 0 ? commands::idle : cmdq.front()) {

  case commands::empathy_up:
    if (current_state.empathy.value + 1 <= 5) {
      current_state.empathy.value += 1;
      current_state.points.value -= 1;
      cmdq.push_back(commands::ui_refresh);
    }
    cmdq.pop_front();
    break;

  case commands::empathy_down:
    if (current_state.empathy.value - 1 >= 0) {
      current_state.empathy.value -= 1;
      current_state.points.value += 1;
      cmdq.push_back(commands::ui_refresh);
    }
    cmdq.pop_front();
    break;

  default:
    break;
  }
}

void DTU::ui::character_sheet::bake_and_send(const ui_state_desc &current_state,
                                             surge::atom::sprite::data_list &sdl,
                                             surge::atom::text::text_draw_data &tdd,
                                             surge::atom::text::glyph_data &tgd) noexcept {
  // TODO
}