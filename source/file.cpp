#include "file.hpp"

#include "log.hpp"

#ifdef SURGE_SYSTEM_IS_POSIX
#  include <fcntl.h>
#endif

#include <filesystem>
#include <optional>

auto surge::validate_path(const std::filesystem::path &path,
                          const char *expected_extension) noexcept -> std::optional<io_error> {

  try {
    if (!std::filesystem::exists(path)) {
      log_all<log_event::error>("The path {} does not exist.", path.string());
      return surge::io_error::file_does_not_exist;
    }

    if (!std::filesystem::is_regular_file(path)) {
      log_all<log_event::error>("The path {} does not point to a regular file.", path.string());
      return surge::io_error::file_is_not_regular;
    }

    if (path.extension() != expected_extension) {
      log_all<log_event::error>("The path {} does not point to a \"{}\" file.", path.string(),
                                expected_extension);
      return surge::io_error::wrong_extension;
    }

    return {};

  } catch (const std::exception &e) {
    std::cout << "Error while validating file path : " << e.what() << std::endl;

    return surge::io_error::unknow_exception;
  }
}

auto surge::load_to_mem(const std::filesystem::path &p, const char *ext, std::size_t bytes,
                        void *buffer) noexcept -> std::optional<io_error> {

  const auto path_validation_result{validate_path(p, ext)};

  if (path_validation_result.has_value()) {
    return path_validation_result;
  }

#ifdef SURGE_SYSTEM_IS_POSIX
  // NOLINTNEXTLINE
  int fd = open(p.c_str(), O_RDONLY);

  if (fd == -1) {
    log_all<log_event::error>("Error while oppening file: {}", std::strerror(errno));
    return io_error::open_error;
  }

  if (read(fd, buffer, bytes) == -1) {
    log_all<log_event::error>("Uanable to read the file {}: {}", p.c_str(), std::strerror(errno));
    close(fd);
    return io_error::read_error;
  }

  close(fd);
#endif

  return {};
}