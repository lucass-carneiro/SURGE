#ifndef SURGE_SAFE_OPS_HPP
#define SURGE_SAFE_OPS_HPP

#include "log.hpp"

#include <optional>

namespace surge {

enum class cast_error { missing_input_type, target_type_too_small, negative_to_unsigned_undefined };

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

} // namespace surge

#endif // SURGE_SAFE_OPS_HPP