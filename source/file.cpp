#include "file.hpp"

#include "allocator.hpp"
#include "logging_system/logging_system.hpp"
#include "options.hpp"

#ifdef SURGE_SYSTEM_IS_POSIX
#  include <fcntl.h>
#elif defined(SURGE_SYSTEM_Windows)
#  include <fcntl.h>
#  include <io.h>
#endif

#include <cstdio>
#include <cstring>

auto surge::validate_path(const char *path, const char *expected_extension) noexcept -> bool {
  using std::printf;

  try {
    const std::filesystem::path fs_opath{path};

    if (!std::filesystem::exists(fs_opath)) {
      log_error("The path {} does not exist.", path);
      return false;
    }

    if (!std::filesystem::is_regular_file(fs_opath)) {
      log_error("The path {} does not point to a regular file.", path);
      return false;
    }

    if (fs_opath.extension() != expected_extension) {
      log_error("The path {} does not point to a {} file.", path, expected_extension);
      return false;
    }

    return true;

  } catch (const std::exception &e) {
    // NOLINTNEXTLINE(ppcoreguidelines-pro-type-vararg)
    printf("Error while validating file path: %s\n", e.what());
    return false;
  }
}

auto surge::load_file(const char *path, const char *ext, bool append_null_byte) noexcept
    -> load_file_return_t {

  try {
    log_info("Loading raw data for file {}. Appending null byte: {}", path, append_null_byte);

    const auto path_validation_result{validate_path(path, ext)};

    if (path_validation_result == false) {
      return {};
    }

    const auto file_size{[&]() {
      if (append_null_byte) {
        return (std::filesystem::file_size(path) + 1);
      } else {
        return std::filesystem::file_size(path);
      }
    }()};

    void *buffer{mi_malloc(file_size)};
    std::memset(buffer, 0, file_size);

    if (buffer == nullptr) {
      log_error("Unable to allocate memory to hold file {}", path);
      return {};
    }

    auto byte_buffer{static_cast<std::byte *>(buffer)};

    if (append_null_byte) {
      byte_buffer[file_size - 1] = std::byte{0};
    }

    if (os_open_read(path, buffer, file_size)) {
      return load_file_return_t{load_file_span{byte_buffer, file_size}};
    } else {
      mi_free(buffer);
      return {};
    }

  } catch (const std::exception &e) {
    log_error("Unable to load file {}: {}", path, e.what());
    return {};
  }
}

#ifdef SURGE_SYSTEM_IS_POSIX

auto surge::os_open_read(const char *path, void *buffer, std::uintmax_t file_size) noexcept
    -> bool {
  // NOLINTNEXTLINE
  int fd = open(path, O_RDONLY);

  if (fd == -1) {
    log_error("Error while oppening file: {}", std::strerror(errno));
    return false;
  }

  if (read(fd, buffer, file_size) == -1) {
    log_error("Uanable to read the file {}: {}", path, std::strerror(errno));
    close(fd);
    return false;
  }

  close(fd);

  return true;
}

#else

// TODO: Error check
auto surge::os_open_read(const std::filesystem::path &p, void *buffer,
                         std::uintmax_t file_size) noexcept -> bool {
  auto file{std::fopen(p.string().c_str(), "rb")};
  std::fread(buffer, 1, file_size, file);
  return true;
}

#endif