#ifndef SURGE_FRAME_TIMER_HPP
#define SURGE_FRAME_TIMER_HPP

#include <chrono>

namespace surge::frame_timer {

using clock_t = std::chrono::steady_clock;
using duration_t = std::chrono::duration<double>;
using time_pt_t = std::chrono::time_point<clock_t>;

struct frame_timer_data {
  time_pt_t begin;
  double elapsed{0};
};

void begin(frame_timer_data &clock_data) noexcept;
void end(frame_timer_data &clock_data) noexcept;
auto duration(const frame_timer_data &clock_data) noexcept -> double;

} // namespace surge::frame_timer

#endif // SURGE_FRAME_TIMER_HPP