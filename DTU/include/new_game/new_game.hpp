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

auto load(surge::deque<surge::u32> &cmdq, surge::atom::sprite::data_list &dl, float ww,
          float wh) noexcept -> int;

void unload(surge::deque<surge::u32> &cmdq, surge::atom::sprite::data_list &dl) noexcept;

void update(surge::deque<surge::u32> &cmdq, surge::atom::sprite::data_list &dl, double dt) noexcept;

void mouse_click(GLFWwindow *window, int button, int action, int mods) noexcept;
void mouse_scroll(GLFWwindow *window, double xoffset, double yoffset) noexcept;

} // namespace DTU::state::new_game

#endif // SURGE_DTU_STATE_NEW_GAME
