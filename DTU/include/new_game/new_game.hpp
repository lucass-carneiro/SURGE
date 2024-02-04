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

enum commands : surge::u32 { idle };

auto load(surge::deque<surge::u32> &cmdq, DTU::sprite::data_list &dl, float ww, float wh) noexcept
    -> int;

void unload(surge::deque<surge::u32> &cmdq, DTU::sprite::data_list &dl) noexcept;

void update(surge::deque<surge::u32> &cmdq, DTU::sprite::data_list &dl, double dt) noexcept;

void keyboard_event(surge::deque<surge::u32> &cmdq, int key, int scancode, int action,
                    int mods) noexcept;

} // namespace DTU::state::new_game

#endif // SURGE_DTU_STATE_NEW_GAME
