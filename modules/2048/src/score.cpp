
#include "score.hpp"

#include "2048.hpp"

void mod_2048::score::draw() noexcept {
  using namespace surge::atom;

  const auto &projection{get_projection()};
  const auto &text_shader{get_txt_shader()};
  const auto &text_buffer{get_text_buffer()};
  const auto &charmap{get_text_charmap()};

  const glm::vec2 pos{359.0f, 47.0f};
  const float scale{1.0f};
  const glm::vec3 color{1.0f, 1.0f, 1.0f};

  const text::draw_data dd{projection, 0, pos, scale, color};

  text::draw(text_shader, text_buffer, charmap, dd, "1234");
}