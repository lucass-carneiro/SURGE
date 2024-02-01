#include "new_game/new_game.hpp"

#include "player/logging.hpp"

auto DTU::state::new_game::load(surge::deque<surge::u32> &, surge::vector<glm::mat4> &,
                                surge::vector<GLuint64> &, surge::vector<float> &, float,
                                float) noexcept -> int {
  log_info("Loading new_game state");
  return 0;
}

void DTU::state::new_game::unload(surge::deque<surge::u32> &, surge::vector<glm::mat4> &,
                                  surge::vector<GLuint64> &) noexcept {
  log_info("Unloading new_game state");
  return;
}

void DTU::state::new_game::update(surge::deque<surge::u32> &, surge::vector<glm::mat4> &,
                                  surge::vector<float> &, double) noexcept {
  // TODO
  return;
}

void DTU::state::new_game::keyboard_event(surge::deque<surge::u32> &, int, int, int, int) noexcept {
  // TODO
  return;
}