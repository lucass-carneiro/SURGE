#include "file.hpp"

#include "allocator.hpp"
#include "log.hpp"
#include "options.hpp"

#ifdef SURGE_SYSTEM_IS_POSIX
#  include <fcntl.h>
#elif defined(SURGE_SYSTEM_Windows)
#  include <fcntl.h>
#  include <io.h>
#endif

#include <cstdio>
#include <cstring>

auto surge::validate_path(const std::filesystem::path &path,
                          const char *expected_extension) noexcept -> bool {

  try {
    if (!std::filesystem::exists(path)) {
#ifdef SURGE_SYSTEM_Windows
      log_error(L"The path {} does not exist.", path.c_str());
#else
      log_error("The path {} does not exist.", path.c_str());
#endif
      return false;
    }

    if (!std::filesystem::is_regular_file(path)) {
#ifdef SURGE_SYSTEM_Windows
      log_error(L"The path {} does not point to a regular file.", path.c_str());
#else
      log_error("The path {} does not point to a regular file.", path.c_str());
#endif
      return false;
    }

    if (path.extension() != expected_extension) {
#ifdef SURGE_SYSTEM_Windows
      log_error(L"The path {} does not point to a file with the correct extension.", path.c_str());
      log_error("Expected extension: {}", expected_extension);
      return false;
#else
      log_error("The path {} does not point to a \"{}\" file.", path.c_str(), expected_extension);
#endif
    }

    return true;

  } catch (const std::exception &e) {
    std::printf("Error while validating file path: %s\n", e.what());

    return false;
  }
}

auto surge::load_file(const std::filesystem::path &p, const char *ext,
                      bool append_null_byte) noexcept -> load_file_return_t {
#ifdef SURGE_SYSTEM_Windows
  log_info(L"Loading raw data for file {}. Appending null byte: {}", p.c_str(), append_null_byte);
#else
  log_info("Loading raw data for file {}. Appending null byte: {}", p.c_str(), append_null_byte);
#endif

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
  std::memset(buffer, 0, file_size);

  if (buffer == nullptr) {
#ifdef SURGE_SYSTEM_Windows
    log_error(L"Unable to allocate memory to hold file {}", p.c_str());
#else
    log_error("Unable to allocate memory to hold file {}", p.c_str());
#endif
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
    log_error("Error while oppening file: {}", std::strerror(errno));
    return false;
  }

  if (read(fd, buffer, file_size) == -1) {
    log_error("Uanable to read the file {}: {}", p.c_str(), std::strerror(errno));
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