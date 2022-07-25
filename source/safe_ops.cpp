#include "safe_ops.hpp"
#include <optional>

auto surge::validate_path(const std::filesystem::path &path,
                          const char *expected_extension) noexcept
    -> std::optional<path_error_type> {

  using tl::unexpected;

  try {
    if (!std::filesystem::exists(path)) {
      log_all<log_event::error>("The path {} does not exist.", path.string());
      return surge::path_error_type::file_does_not_exist;
    }

    if (!std::filesystem::is_regular_file(path)) {
      log_all<log_event::error>("The path {} does not point to a regular file.",
                                path.string());
      return surge::path_error_type::file_is_not_regular;
    }

    if (path.extension() != expected_extension) {
      log_all<log_event::error>("The path {} does not point to a \"{}\" file.",
                                path.string(), expected_extension);
      return surge::path_error_type::file_is_not_nut;
    }

    return {};

  } catch (const std::exception &e) {
    std::cout << "Error while validating file path : " << e.what() << std::endl;

    return surge::path_error_type::unknow_exception;
  }
}