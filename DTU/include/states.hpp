#ifndef SURGE_DTU_STATES_HPP
#define SURGE_DTU_STATES_HPP

#include "player/integer_types.hpp"

namespace DTU::state_machine {

using state_t = surge::u32;
enum states : surge::u32 { no_state, exit_game, main_menu, new_game, count };

void push_state(state_t state) noexcept;
void transition(float ww, float wh) noexcept;

auto get_a() noexcept -> state_t;
auto get_b() noexcept -> state_t;
auto to_str(state_t state) noexcept -> const char *;

} // namespace DTU::state_machine

#endif // SURGE_DTU_STATES_HPP