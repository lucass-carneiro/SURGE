#include "file.hpp"

#include "allocator.hpp"
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

auto surge::load_file(const std::filesystem::path &p, const char *ext,
                      bool append_null_byte) noexcept -> load_file_return_t {
  glog<log_event::message>("Loading raw data for file {}. Appending null byte: {}", p.c_str(),
                           append_null_byte);

  const auto path_validation_result{validate_path(p, ext)};

  if (path_validation_result == false) {
    return {};
  }

  const auto file_size{[&]() {
    if (append_null_byte)
      return (std::filesystem::file_size(p) + 1);
    else
      return std::filesystem::file_size(p);
  }()};
  void *buffer{mi_malloc(file_size)};

  if (buffer == nullptr) {
    glog<log_event::error>("Unable to allocate memory to hold file {}", p.c_str());
    return {};
  }

  auto byte_buffer{static_cast<std::byte *>(buffer)};

  if (append_null_byte) {
    byte_buffer[file_size - 1] = std::byte{0};
  }

  if (os_open_read(p, buffer, file_size)) {
    return load_file_return_t{load_file_span{byte_buffer, file_size}};
  } else {
    mi_free(buffer);
    return {};
  }
}

#ifdef SURGE_SYSTEM_IS_POSIX

auto surge::os_open_read(const std::filesystem::path &p, void *buffer,
                         std::uintmax_t file_size) noexcept -> bool {
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
}

#else
#  error "No os_open_read implementation for this system is available. Please implement it here"
#endif