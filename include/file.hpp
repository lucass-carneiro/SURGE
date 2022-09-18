#ifndef SURGE_FILE_HPP
#define SURGE_FILE_HPP

#include "allocators/allocators.hpp"
#include "options.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>

namespace surge {

enum class io_error : unsigned {
  file_does_not_exist,
  file_is_not_regular,
  wrong_extension,
  open_error,
  read_error,
  unknow_exception
};

/**
 * @brief Validates a path.
 *+
 * @param path The path to validate.
 * @return Nothing if the path is valid, an error code otherwise.
 */
auto validate_path(const std::filesystem::path &path, const char *expected_extension) noexcept
    -> std::optional<io_error>;

struct file_handle {
  void *data;
  std::size_t size;
  std::size_t cursor;
};

/**
 * @brief Loads bytes of a file to memory
 *
 * @param p Path to the file.
 * @param ext File extension.
 * @param bytes Number of bytes to read from the file
 * @param buffer Buffer to store the read data
 * @return std::optional<io_error>
 */
auto load_to_mem(const std::filesystem::path &p, const char *ext, std::size_t bytes,
                 void *buffer) noexcept -> std::optional<io_error>;

} // namespace surge

#endif // SURGE_FILE_HPP