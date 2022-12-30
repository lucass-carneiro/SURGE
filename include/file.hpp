#ifndef SURGE_FILE_HPP
#define SURGE_FILE_HPP

#include "allocators/allocators.hpp"
#include "options.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>

#ifdef SURGE_DEBUG_MEMORY
#  include <gsl/gsl-lite.hpp>
#else
#  include <span>
#endif

namespace surge {

enum class io_error : unsigned {
  file_does_not_exist,
  file_is_not_regular,
  wrong_extension,
  open_error,
  read_error,
  command_line_error,
  unknow_exception
};

/**
 * @brief Validates a path.
 *+
 * @param path The path to validate.
 * @return Nothing if the path is valid, an error code otherwise.
 */
auto validate_path(const std::filesystem::path &path, const char *expected_extension) noexcept
    -> bool;

/**
 * @brief Opens and reads a whole file using OS specific calls
 *
 * @param p Path to the file
 * @param buffer Buffer to hold the file
 * @param file_size Size of the file
 * @return true If the file was loaded sucesfully
 * @return false If there was an error loading the file.
 */
auto os_open_read(const std::filesystem::path &p, void *buffer, std::uintmax_t file_size) noexcept
    -> bool;

#ifdef SURGE_DEBUG_MEMORY
using load_file_span = gsl::span<std::byte>;
#else
using load_file_span = std::span<std::byte>;
#endif

using load_file_return_t = std::optional<load_file_span>;

/**
 * @brief Loads a whole file to memory using the specified allocator.
 *
 * @tparam alloc_t Type of the allocator to use
 * @param allocator Allocator instance to use.
 * @param p Path to the file
 * @param ext File extension
 * @return load_file_return_t An optional std/gsl span of the file bytes
 */
template <surge_allocator alloc_t, bool append_null_byte = false>
inline auto load_file(alloc_t *allocator, const std::filesystem::path &p, const char *ext) noexcept
    -> load_file_return_t {
  glog<log_event::message>("Loading raw data for file {}. Appending null byte: {}", p.c_str(),
                           append_null_byte);

  const auto path_validation_result{validate_path(p, ext)};

  if (path_validation_result == false) {
    return {};
  }

  const auto file_size{[&]() {
    if constexpr (append_null_byte)
      return (std::filesystem::file_size(p) + 1);
    else
      return std::filesystem::file_size(p);
  }()};
  void *buffer{allocator->malloc(file_size)};

  if (buffer == nullptr) {
    glog<log_event::error>("Unable to allocate memory to hold file {}", p.c_str());
    return {};
  }

  auto byte_buffer{static_cast<std::byte *>(buffer)};

  if constexpr (append_null_byte) {
    byte_buffer[file_size - 1] = std::byte{0};
  }

  if (os_open_read(p, buffer, file_size)) {
    return load_file_return_t{load_file_span{byte_buffer, file_size}};
  } else {
    allocator->free(buffer);
    return {};
  }
}

} // namespace surge

#endif // SURGE_FILE_HPP