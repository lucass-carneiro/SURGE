#ifndef SURGE_SAFE_OPS_HPP
#define SURGE_SAFE_OPS_HPP

#include "log.hpp"

#include <concepts>
#include <filesystem>
#include <limits>
#include <optional>
#include <string>
#include <tl/expected.hpp>
#include <type_traits>

namespace surge {

enum class cast_error {
  missing_input_type,
  target_type_too_small,
  negative_to_unsigned_undefined
};

template <std::integral target_type, std::integral original_type>
constexpr inline auto safe_cast(original_type value) noexcept
    -> tl::expected<target_type, cast_error> {
  using namespace tl;

  if constexpr ((std::is_unsigned<original_type>::value &&
                 std::is_unsigned<target_type>::value) ||
                (std::is_signed<original_type>::value &&
                 std::is_signed<target_type>::value) ||
                (std::is_unsigned<original_type>::value &&
                 std::is_signed<target_type>::value)) {

    if (value <= std::numeric_limits<target_type>::max()) {
      return static_cast<target_type>(value);

    } else {
      log_all<log_event::error>(
          "Attempt to cast {} to a target type to small to "
          "hold the original value.",
          value);
      return unexpected(cast_error::target_type_too_small);
    }
  } else {
    log_all<log_event::error>(
        "Casting a negative number ({}) to a unsigned type is undefined.",
        value);
    return unexpected(cast_error::negative_to_unsigned_undefined);
  }
}

enum class path_error_type : int {
  file_does_not_exist,
  file_is_not_regular,
  file_is_not_nut,
  unknow_exception
};

/**
 * Validates a path.
 *
 * @param path The path to validate.
 * @return Nothing if the path is valid, an error code otherwise.
 */
auto validate_path(const std::filesystem::path &path,
                   const char *expected_extension) noexcept
    -> std::optional<path_error_type>;

} // namespace surge

#endif // SURGE_SAFE_OPS_HPP