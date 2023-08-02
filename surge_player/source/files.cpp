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
#  include <io.h>
#endif

#include <cstring>
#include <filesystem>

#ifdef SURGE_SYSTEM_IS_POSIX

auto os_open_read(const char *path, void *buffer, std::uintmax_t file_size) noexcept -> bool {
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

#else
// TODO: Error check, unsafe on windows. Do better
auto surge::os_open_read(const char *p, void *buffer, std::uintmax_t file_size) noexcept -> bool {
  auto file{std::fopen(p, "rb")};
  std::fread(buffer, 1, file_size, file);
  return true;
}
#endif

auto surge::files::load_file(const char *path, bool append_null_byte) noexcept
    -> load_file_return_t {

  try {
    log_info("Loading raw data for file %s. Appending null byte: %s", path,
             append_null_byte ? "true" : "false");

    const auto path_valid{validate_path(path)};
    if (!path_valid) {
      return {};
    }

    const auto file_size{[&]() {
      if (append_null_byte) {
        return (std::filesystem::file_size(path) + 1);
      } else {
        return std::filesystem::file_size(path);
      }
    }()};

    void *buffer{allocators::mimalloc::malloc(file_size)};
    std::memset(buffer, 0, file_size);

    if (!buffer) {
      log_error("Unable to allocate memory to hold file %s", path);
      return {};
    }

    auto byte_buffer{static_cast<std::byte *>(buffer)};

    if (append_null_byte) {
      byte_buffer[file_size - 1] = std::byte{0};
    }

    if (os_open_read(path, buffer, file_size)) {
      return load_file_return_t{load_file_span{byte_buffer, file_size}};
    } else {
      allocators::mimalloc::free(buffer);
      return {};
    }

  } catch (const std::exception &e) {
    log_error("Unable to load file %s: %s", path, e.what());
    return {};
  }
}

void surge::files::free_file(load_file_span &file) noexcept {
  allocators::mimalloc::free(static_cast<void *>(file.data()));
}

auto surge::files::validate_path(const char *path) noexcept -> bool {
  using std::printf;

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