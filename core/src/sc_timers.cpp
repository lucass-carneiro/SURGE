#include "sc_timers.hpp"

surge::timers::GenericTimer::GenericTimer() noexcept { begin = clock_t::now(); }

void surge::timers::GenericTimer::start() noexcept { begin = clock_t::now(); }

auto surge::timers::GenericTimer::stop() noexcept -> double {
  last_elapsed = duration_t{clock_t::now() - begin}.count();
  return last_elapsed;
}

[[nodiscard]] auto surge::timers::GenericTimer::elapsed() noexcept -> double {
  return last_elapsed;
}

[[nodiscard]] auto surge::timers::GenericTimer::since_start() noexcept -> double {
  return duration_t{clock_t::now() - begin}.count();
}