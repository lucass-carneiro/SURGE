#ifndef DTU_UI_HPP
#define DTU_UI_HPP

#include "player/integer_types.hpp"
#include "player/window.hpp"
#include "type_aliases.hpp"

namespace DTU::ui {

/*
 * Detect if a point is inside a rectangle
 */
static inline auto point_in_rect(const glm::vec2 &point, const glm::vec4 &rect) noexcept {
  const auto x{point[0]};
  const auto y{point[1]};

  const auto x0{rect[0]};
  const auto xf{rect[0] + rect[2]};

  const auto y0{rect[1]};
  const auto yf{rect[1] + rect[3]};

  const auto in_x_range{x0 < x && x < xf};
  const auto in_y_range{y0 < y && y < yf};

  return in_x_range && in_y_range;
}

struct ui_state {
  GLFWwindow *window;
  surge::i32 active;
  surge::i32 hot;
};

struct draw_data {
  glm::vec2 pos;
  glm::vec2 scale;
  float z;
  float alpha;
};

struct button_skin {
  GLuint64 handle_release;
  GLuint64 handle_select;
  GLuint64 handle_press;
};

auto button(surge::i32 id, ui_state &state, draw_data &dd, sdb_t &sdb,
            const button_skin &bs) noexcept -> bool;

} // namespace DTU::ui

#endif // DTU_UI_HPP