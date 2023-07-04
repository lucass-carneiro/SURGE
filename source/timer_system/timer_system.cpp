#include "timer_system/timer_system.hpp"

#include <cstdio>

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

void generic_timer_delete(surge::generic_timer *this_) noexcept { delete this_; }

void generic_timer_start(surge::generic_timer *this_) noexcept { this_->start(); }

void generic_timer_stop(surge::generic_timer *this_) noexcept { this_->stop(); }

auto generic_timer_elapsed(surge::generic_timer *this_) noexcept -> double {
  return this_->elapsed();
}
}