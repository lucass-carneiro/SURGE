#ifndef SURGE_FRAME_TIMER_HPP
#define SURGE_FRAME_TIMER_HPP

#include <chrono>

namespace surge::frame_timer {

using clock_t = std::chrono::steady_clock;
using duration_t = std::chrono::duration<double>;
using time_pt_t = std::chrono::time_point<clock_t>;

extern struct frame_timer_data {
  time_pt_t begin;
  double elapsed{0};
} clock_data;

void begin() noexcept;
void end() noexcept;
auto duration() noexcept -> double;

} // namespace surge::frame_timer

#endif // SURGE_FRAME_TIMER_HPP