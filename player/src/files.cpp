#include "files.hpp"

#include "allocators.hpp"
#include "logging.hpp"
#include "options.hpp"

#ifdef SURGE_SYSTEM_IS_POSIX
#  include <fcntl.h>
#elif defined(SURGE_SYSTEM_Windows)
#  include <fcntl.h>
#  include <gsl/gsl-lite.hpp>
#  include <io.h>
#endif

#include <array>
#include <cstring>
#include <filesystem>
#include <limits>

auto surge::files::validate_path(const char *path) noexcept -> bool {
  try {
    const std::filesystem::path fs_opath{path};

    if (!std::filesystem::exists(fs_opath)) {
      log_error("The file %s does not exist.", path);
      return false;
    }

    if (!std::filesystem::is_regular_file(fs_opath)) {
      log_error("The path %s does not point to a regular file.", path);
      return false;
    }

    return true;

  } catch (const std::exception &e) {
    log_error("Error while validating file %s: %s", path, e.what());
    return false;
  }
}

#ifdef SURGE_SYSTEM_Windows

auto os_open_read(const char *path, void *buffer, unsigned int file_size) noexcept -> bool {
  std::array<char, 256> error_msg_buff{};
  error_msg_buff.fill('\0');

  int fd = 0;
  if (_sopen_s(&fd, path, _O_RDONLY | _O_BINARY, _SH_DENYWR, _S_IREAD) != 0) {
    strerror_s(error_msg_buff.data(), error_msg_buff.size(), errno);
    log_error("Error while oppening file %s: %s", path, error_msg_buff.data());
    return false;
  }

  if (_read(fd, buffer, file_size) == -1) {
    strerror_s(error_msg_buff.data(), error_msg_buff.size(), errno);
    log_error("Uanable to read the file %s: %s", path, error_msg_buff.data());
    _close(fd);
    return false;
  }

  _close(fd);

  return true;
}

#else

auto os_open_read(const char *path, void *buffer, unsigned int file_size) noexcept -> bool {
  // NOLINTNEXTLINE
  int fd = open(path, O_RDONLY);

  if (fd == -1) {
    log_error("Error while oppening file: %s", std::strerror(errno));
    return false;
  }

  if (read(fd, buffer, file_size) == -1) {
    log_error("Uanable to read the file %s: %s", path, std::strerror(errno));
    close(fd);
    return false;
  }

  close(fd);

  return true;
}

#endif

auto surge::files::load_file(const char *path, bool append_null_byte) noexcept -> file {
  try {
    log_info("Loading raw data for file %s. Appending null byte: %s", path,
             append_null_byte ? "true" : "false");

    if (!validate_path(path)) {
      return tl::unexpected(file_error::invalid_path);
    }

    std::uintmax_t file_size{std::filesystem::file_size(path)};
    if (append_null_byte) {
      file_size += 1;
    }

    file_data_t buffer(file_size);
    buffer.reserve(file_size);
    std::fill(buffer.begin(), buffer.end(), std::byte{0});

    if (os_open_read(path, buffer.data(), file_size)) {
      return buffer;
    } else {
      return tl::unexpected(file_error::read_error);
    }

  } catch (const std::exception &e) {
    log_error("Unable to load file %s: %s", path, e.what());
    return tl::unexpected(file_error::unknow_error);
  }
}