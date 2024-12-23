#include "sc_timers.hpp"

surge::timers::generic_timer::generic_timer() noexcept { begin = clock_t::now(); }

void surge::timers::generic_timer::start() noexcept { begin = clock_t::now(); }

auto surge::timers::generic_timer::stop() noexcept -> double {
  last_elapsed = duration_t{clock_t::now() - begin}.count();
  return last_elapsed;
}

[[nodiscard]] auto surge::timers::generic_timer::elapsed() noexcept -> double {
  return last_elapsed;
}