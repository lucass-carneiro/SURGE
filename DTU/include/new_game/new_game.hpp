#ifndef SURGE_DTU_STATE_NEW_GAME
#define SURGE_DTU_STATE_NEW_GAME

#include "player/container_types.hpp"
#include "player/text.hpp"

// clang-format off
#include "DTU.hpp"
#include "states.hpp"
// clang-format on

#include <glm/glm.hpp>

namespace DTU::state::new_game {

enum commands : surge::u32 {
  idle,

  empathy_up,
  empathy_down,

  introspection_up,
  introspection_down,

  linguistics_up,
  linguistics_down,

  reasoning_up,
  reasoning_down,

  fitness_up,
  fitness_down,

  agility_up,
  agility_down,
};

struct ui_state {
  surge::u8 empathy_rank{0};
  surge::u8 introspection_rank{0};
  surge::u8 reasoning_rank{0};
  surge::u8 linguistics_rank{0};
  surge::u8 fitness_rank{0};
  surge::u8 agility_rank{0};
  surge::u8 remaining_points{12};
};

auto load(surge::deque<surge::u32> &cmdq, surge::atom::sprite::data_list &dl, float ww,
          float wh) noexcept -> int;

void unload(surge::deque<surge::u32> &cmdq, surge::atom::sprite::data_list &dl) noexcept;

void update(GLFWwindow *window, surge::deque<surge::u32> &cmdq, surge::atom::sprite::data_list &dl,
            surge::atom::text::text_draw_data &ptb, surge::atom::text::text_draw_data &etb,
            surge::atom::text::glyph_data &tgd, double dt) noexcept;

void mouse_click(surge::deque<surge::u32> &cmdq, GLFWwindow *window, int button, int action,
                 int mods) noexcept;
void mouse_scroll(surge::deque<surge::u32> &cmdq, GLFWwindow *window, double xoffset,
                  double yoffset) noexcept;

} // namespace DTU::state::new_game

#endif // SURGE_DTU_STATE_NEW_GAME
