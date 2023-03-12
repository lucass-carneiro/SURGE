#ifndef SURGE_SAFE_OPS_HPP
#define SURGE_SAFE_OPS_HPP

#include "log.hpp"

#include <concepts>
#include <limits>
#include <optional>

namespace surge {

template <std::integral target_type, std::integral original_type>
[[nodiscard]] constexpr inline auto safe_cast(original_type value) noexcept
    -> std::optional<target_type> {

  if constexpr ((std::is_unsigned<original_type>::value && std::is_unsigned<target_type>::value)
                || (std::is_signed<original_type>::value && std::is_signed<target_type>::value)
                || (std::is_unsigned<original_type>::value && std::is_signed<target_type>::value)) {

    if (value <= std::numeric_limits<target_type>::max()) {
      return static_cast<target_type>(value);

    } else {
      glog<log_event::error>("Attempt to cast {} to a target type to small to "
                             "hold the original value.",
                             value);
      return {};
    }
  } else {
    glog<log_event::error>("Casting a negative number ({}) to a unsigned type is undefined.",
                           value);
    return {};
  }
}

template <std::floating_point T> [[nodiscard]] constexpr auto isapprox(T x, T y, T atol = T{0})
    -> bool {
  const auto rtol{atol > T{0} ? T{0} : std::sqrt(std::numeric_limits<T>::epsilon())};
  const auto abs_x{std::abs(x)};
  const auto abs_y{std::abs(y)};
  const auto abs_x_m_y{std::abs(x - y)};
  return abs_x_m_y <= std::max(atol, rtol * std::max(abs_x, abs_y));
}

} // namespace surge

#endif // SURGE_SAFE_OPS_HPP