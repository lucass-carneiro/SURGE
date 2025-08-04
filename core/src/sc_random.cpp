#include "sc_random.hpp"

static inline auto rotl(const surge::u64 x, const int k) -> surge::u64 {
  return (x << k) | (x >> (64 - k));
}

auto surge::random::Xoshiro128::next_pp() -> u64 {
  /* Xoshiro128++ by David Blackman and Sebastiano Vigna (vigna@acm.org), 2019
   * see https://prng.di.unimi.it/ and https://prng.di.unimi.it/xoshiro256plusplus.c
   */
  const u64 result = rotl(state_pp[0] + state_pp[3], 23) + state_pp[0];

  const u64 t = state_pp[1] << 17;

  state_pp[2] ^= state_pp[0];
  state_pp[3] ^= state_pp[1];
  state_pp[1] ^= state_pp[2];
  state_pp[0] ^= state_pp[3];

  state_pp[2] ^= t;

  state_pp[3] = rotl(state_pp[3], 45);

  return result;
}

auto surge::random::Xoshiro128::next_p() -> u64 {
  /* Xoshiro128+ by David Blackman and Sebastiano Vigna (vigna@acm.org), 2019
   * see https://prng.di.unimi.it/ and https://prng.di.unimi.it/xoshiro256plus.c
   */
  const uint64_t result = state_p[0] + state_p[3];

  const uint64_t t = state_p[1] << 17;

  state_p[2] ^= state_p[0];
  state_p[3] ^= state_p[1];
  state_p[1] ^= state_p[2];
  state_p[0] ^= state_p[3];

  state_p[2] ^= t;

  state_p[3] = rotl(state_p[3], 45);

  return result;
}

surge::random::Xoshiro128::Xoshiro128(const State &seed) : state_pp{seed}, state_p{seed} {}

auto surge::random::Xoshiro128::next_int() -> u64 { return next_pp(); }

auto surge::random::Xoshiro128::next_int_in_range_inc(const u64 min, const u64 max) -> u64 {
  if (min == max) {
    return min;
  } else {
    return min + (next_pp() % (max - min + 1));
  }
}

auto surge::random::Xoshiro128::next_float() -> float {
  // Generate a random in [0.0, 1.0]
  const auto x{next_p()};
  const auto top24{static_cast<u32>(x >> 40)};
  return static_cast<float>(top24) / ((1U << 24) - 1);
}

auto surge::random::Xoshiro128::next_float_in_range_inc(const float min, const float max) -> float {
  // 1. Generate a random in [0.0, 1.0]
  const auto u{next_float()};

  // 2. Scale it to desired range
  return min + (max - min) * u;
}