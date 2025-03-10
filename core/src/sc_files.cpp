#include "sc_files.hpp"

#include "sc_logging.hpp"

#include <fcntl.h>

#ifdef SURGE_SYSTEM_Windows
#  include <gsl/gsl-lite.hpp>
#  include <io.h>
#endif

#include <exception>
#include <filesystem>

#ifdef SURGE_SYSTEM_Windows
#  include <array>
#endif

auto surge::files::is_path_valid(const char *path) -> bool {
  using namespace std::filesystem;

  try {
    const std::filesystem::path fs_opath{path};

    if (!exists(fs_opath)) {
      log_error("The file {} does not exist.", path);
      return false;
    }

    if (!is_regular_file(fs_opath)) {
      log_error("The path {} does not point to a regular file.", path);
      return false;
    }

    return true;

  } catch (const std::exception &e) {
    log_error("Error while validating file {}: {}", path, e.what());
    return false;
  }
}

#ifdef SURGE_SYSTEM_Windows

static auto os_open_read(const char *path, void *buffer, unsigned int file_size) -> bool {
  std::array<char, 256> error_msg_buff{};
  error_msg_buff.fill('\0');

  int fd = 0;
  if (_sopen_s(&fd, path, _O_RDONLY | _O_BINARY, _SH_DENYWR, _S_IREAD) != 0) {
    strerror_s(error_msg_buff.data(), error_msg_buff.size(), errno);
    log_error("Error while oppening file {}: {}", path, error_msg_buff.data());
    return false;
  }

  if (_read(fd, buffer, file_size) == -1) {
    strerror_s(error_msg_buff.data(), error_msg_buff.size(), errno);
    log_error("Uanable to read the file {}: {}", path, error_msg_buff.data());
    _close(fd);
    return false;
  }

  _close(fd);

  return true;
}

#else

static auto os_open_read(const char *path, void *buffer, unsigned int file_size) -> bool {
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

#endif

auto surge::files::as_bytes(const char *path, bool append_null_byte)
    -> Result<containers::mimalloc::Vector<std::byte>> {
  try {
    log_info("Loading raw data for file {}. Appending null byte: {}", path,
             append_null_byte ? "true" : "false");

    if (!is_path_valid(path)) {
      return Err{Error::invalid_path};
    }

    const auto base_file_size{static_cast<unsigned int>(std::filesystem::file_size(path))};
    const auto file_size{append_null_byte ? base_file_size + 1 : base_file_size};

    containers::mimalloc::Vector<std::byte> buffer(file_size);
    buffer.reserve(file_size);
    std::fill(buffer.begin(), buffer.end(), std::byte{0});

    if (os_open_read(path, buffer.data(), file_size)) {
      return buffer;
    } else {
      return Err{Error::read_error};
    }

  } catch (const std::exception &e) {
    log_error("Unable to load file {}: {}", path, e.what());
    return Err{Error::unknow_error};
  }
}

auto surge::files::as_bytes(const char *path, const allocators::scoped::Lifetimes &lifetime,
                            bool append_null_byte)
    -> Result<containers::scoped::Vector<std::byte>> {
  try {
    log_info("Loading raw data for file {}. Appending null byte: {}", path,
             append_null_byte ? "true" : "false");

    if (!is_path_valid(path)) {
      return Err{Error::invalid_path};
    }

    const auto base_file_size{static_cast<unsigned int>(std::filesystem::file_size(path))};
    const auto file_size{append_null_byte ? base_file_size + 1 : base_file_size};

    containers::scoped::Vector<std::byte> buffer{file_size, lifetime};
    buffer.reserve(file_size);
    std::fill(buffer.begin(), buffer.end(), std::byte{0});

    if (os_open_read(path, buffer.data(), file_size)) {
      return buffer;
    } else {
      return Err{Error::read_error};
    }
  } catch (const std::exception &e) {
    log_error("Unable to load file {}: {}", path, e.what());
    return Err{Error::unknow_error};
  }
}