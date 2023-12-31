
#include "score.hpp"

#include "2048.hpp"

const glm::vec2 g_score_box_pos_pos{360.0f, 47.0f};
const glm::vec2 g_best_score_box_pos_pos{440.0f, 47.0f};
const glm::vec3 g_score_text_color{1.0f, 1.0f, 1.0f};

void mod_2048::score::draw() noexcept {
  using namespace surge::atom;

  const auto &projection{get_projection()};
  const auto &text_shader{get_txt_shader()};
  const auto &text_buffer{get_text_buffer()};
  const auto &charmap{get_text_charmap()};

  const auto score{get_game_score()};
  const auto best_score{get_best_score()};

  text::draw_data dd{projection, 0, g_score_box_pos_pos, 1.0f, g_score_text_color};
  text::draw(text_shader, text_buffer, charmap, dd, score);

  dd.position = g_best_score_box_pos_pos;
  text::draw(text_shader, text_buffer, charmap, dd, best_score);
}