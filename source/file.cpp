#include "file.hpp"

#include "log.hpp"

#ifdef SURGE_SYSTEM_IS_POSIX
#  include <fcntl.h>
#endif

auto surge::validate_path(const std::filesystem::path &path,
                          const char *expected_extension) noexcept -> bool {

  try {
    if (!std::filesystem::exists(path)) {
      glog<log_event::error>("The path {} does not exist.", path.string());
      return false;
    }

    if (!std::filesystem::is_regular_file(path)) {
      glog<log_event::error>("The path {} does not point to a regular file.", path.string());
      return false;
    }

    if (path.extension() != expected_extension) {
      glog<log_event::error>("The path {} does not point to a \"{}\" file.", path.string(),
                             expected_extension);
      return false;
    }

    return true;

  } catch (const std::exception &e) {
    std::cout << "Error while validating file path : " << e.what() << std::endl;

    return false;
  }
}

auto surge::os_open_read(const std::filesystem::path &p, void *buffer,
                         std::uintmax_t file_size) noexcept -> bool {
#ifdef SURGE_SYSTEM_IS_POSIX
  // NOLINTNEXTLINE
  int fd = open(p.c_str(), O_RDONLY);

  if (fd == -1) {
    glog<log_event::error>("Error while oppening file: {}", std::strerror(errno));
    return false;
  }

  if (read(fd, buffer, file_size) == -1) {
    glog<log_event::error>("Uanable to read the file {}: {}", p.c_str(), std::strerror(errno));
    close(fd);
    return false;
  }

  close(fd);

  return true;
#else
#  error "OS specific open/read functions not implemented"
#endif
}