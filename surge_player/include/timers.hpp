#ifndef SURGE_TIMERS_HPP
#define SURGE_TIMERS_HPP

#include <chrono>

namespace surge::timers {

class generic_timer {
public:
  using clock_t = std::chrono::steady_clock;
  using duration_t = std::chrono::duration<double>;
  using time_pt_t = std::chrono::time_point<clock_t>;

  generic_timer() noexcept;

  void start() noexcept;
  [[nodiscard]] auto stop() noexcept -> double;
  [[nodiscard]] auto elapsed() noexcept -> double;

private:
  time_pt_t begin;
  double last_elapsed{0};
};

} // namespace surge::timers

#endif // SURGE_TIMERS_HPP