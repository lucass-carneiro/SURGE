#include "files.hpp"

#include "allocators.hpp"
#include "logging.hpp"
#include "options.hpp"

// clang-format off
#include <yaml-cpp/yaml.h>
// clang-format on

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

auto surge::files::get_file_size(const char *path) noexcept
    -> tl::expected<file_size_t, file_error> {
  const auto size = std::filesystem::file_size(path);

  // Parenthesis are required becaus windows defines a max macro.
  // See https://stackoverflow.com/a/27443191
  constexpr const auto max_file_size{(std::numeric_limits<unsigned int>::max)()};

  if (size > max_file_size) {
    log_error("The file %s has size %llu, which exceeds the maximun capacity of %u.", path, size,
              max_file_size);
    return tl::unexpected(file_error::size_truncation);
  } else {
    return gsl::narrow_cast<unsigned int>(size);
  }
}

#else

auto surge::files::get_file_size(const char *path) noexcept
    -> tl::expected<std::uintmax_t, file_error> {
  return std::filesystem::file_size(path);
}

#endif

#ifdef SURGE_SYSTEM_Windows
auto os_open_read(const char *path, void *buffer, unsigned int file_size) noexcept -> bool {
  std::array<char, 256> error_msg_buff{};
  error_msg_buff.fill('\0');

  int fd = 0;
  if (_sopen_s(&fd, path, _O_RDONLY, _SH_DENYWR, _S_IREAD) != 0) {
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
#endif

auto surge::files::load_file(const char *path, bool append_null_byte) noexcept -> file {
  try {
    log_info("Loading raw data for file %s. Appending null byte: %s", path,
             append_null_byte ? "true" : "false");

    if (!validate_path(path)) {
      return tl::unexpected(file_error::invalid_path);
    }

    const auto original_file_size{get_file_size(path)};
    if (!original_file_size) {
      return tl::unexpected(original_file_size.error());
    }

#ifdef SURGE_SYSTEM_Windows
    unsigned int file_size{*original_file_size};
#else
    std::uintmax_t file_size{*original_file_size};
#endif
    if (append_null_byte) {
      file_size += 1;
    }

    eastl::vector<std::byte, allocators::eastl::gp_allocator> buffer(file_size);
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