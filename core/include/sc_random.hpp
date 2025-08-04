#ifndef SURGE_CORE_RANDOM_HPP
#define SURGE_CORE_RANDOM_HPP

#include "sc_integer_types.hpp"

#include <array>

namespace surge::random {

class Xoshiro128 {
public:
  using State = std::array<u64, 4>;

  Xoshiro128(const State &seed);

  auto next_int() -> u64;
  auto next_int_in_range_inc(const u64 min, const u64 max) -> u64;

  auto next_float() -> float;
  auto next_float_in_range_inc(const float min, const float max) -> float;

private:
  State state_pp{};
  State state_p{};

  auto next_pp() -> u64;
  auto next_p() -> u64;
};

} // namespace surge::random

#endif // SURGE_CORE_RANDOM_HPP