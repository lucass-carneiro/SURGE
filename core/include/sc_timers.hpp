#ifndef SURGE_CORE_TIMERS_HPP
#define SURGE_CORE_TIMERS_HPP

#include <chrono>

namespace surge::timers {

class generic_timer {
public:
  using clock_t = std::chrono::steady_clock;
  using duration_t = std::chrono::duration<double>;
  using time_pt_t = std::chrono::time_point<clock_t>;

  generic_timer() noexcept;

  void start() noexcept;
  auto stop() noexcept -> double;
  [[nodiscard]] auto elapsed() noexcept -> double;
  [[nodiscard]] auto since_start() noexcept -> double;

private:
  time_pt_t begin;
  double last_elapsed{0};
};

} // namespace surge::timers

#endif // SURGE_CORE_TIMERS_HPP