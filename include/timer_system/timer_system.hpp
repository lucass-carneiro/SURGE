#ifndef SURGE_TIMER_SYSTEM_HPP
#define SURGE_TIMER_SYSTEM_HPP

#include "frame_timer.hpp"
#include "generic_timer.hpp"

extern "C" {

[[nodiscard]] auto generic_timer_new() noexcept -> surge::generic_timer *;
void generic_timer_delete(surge::generic_timer *this_) noexcept;

void generic_timer_start(surge::generic_timer *this_) noexcept;
void generic_timer_stop(surge::generic_timer *this_) noexcept;
auto generic_timer_elapsed(surge::generic_timer *this_) noexcept -> double;
}

#endif