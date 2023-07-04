#ifndef SURGE_GENERIC_TIMER_HPP
#define SURGE_GENERIC_TIMER_HPP

#include <chrono>

namespace surge {

class generic_timer {
public:
  using clock_t = std::chrono::steady_clock;
  using duration_t = std::chrono::duration<double>;
  using time_pt_t = std::chrono::time_point<clock_t>;

  generic_timer() = default;

  inline void start() noexcept { begin = std::chrono::steady_clock::now(); }
  inline void stop() noexcept { end = std::chrono::steady_clock::now(); }

  [[nodiscard]] inline auto elapsed() const noexcept -> double {
    return duration_t{end - begin}.count();
  }

private:
  time_pt_t begin;
  time_pt_t end;
};

} // namespace surge

#endif // SURGE_GENERIC_TIMER_HPP