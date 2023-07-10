#include "timer_system/timer_system.hpp"

#include <cstdio>

surge::frame_timer::frame_timer_data surge::frame_timer::clock_data
    = {std::chrono::steady_clock::now(), 0.0};

void surge::frame_timer::begin() noexcept { clock_data.begin = std::chrono::steady_clock::now(); }

void surge::frame_timer::end() noexcept {
  const duration_t dt{std::chrono::steady_clock::now() - clock_data.begin};
  clock_data.elapsed = dt.count();
}

auto surge::frame_timer::duration() noexcept -> double { return clock_data.elapsed; }

extern "C" {

auto generic_timer_new() noexcept -> surge::generic_timer * {
  using std::printf;

  try {
    auto ptr{new surge::generic_timer()};
    return ptr;
  } catch (const std::exception &e) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    printf("Unable to create new generic timer object: %s\n", e.what());
    return nullptr;
  }
}

void generic_timer_delete(surge::generic_timer *this_) noexcept {
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
  delete this_;
}

void generic_timer_start(surge::generic_timer *this_) noexcept { this_->start(); }

void generic_timer_stop(surge::generic_timer *this_) noexcept { this_->stop(); }

auto generic_timer_elapsed(surge::generic_timer *this_) noexcept -> double {
  return this_->elapsed();
}
}