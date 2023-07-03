#ifndef SURGE_FILE_HPP
#define SURGE_FILE_HPP

#include "options.hpp"

#include <filesystem>
#include <optional>

#ifdef SURGE_DEBUG_MEMORY
#  include <gsl/gsl-lite.hpp>
#else
#  include <span>
#endif

namespace surge {

auto validate_path(const char *path, const char *expected_extension) noexcept -> bool;

#ifdef SURGE_DEBUG_MEMORY
using load_file_span = gsl::span<std::byte>;
#else
using load_file_span = std::span<std::byte>;
#endif

using load_file_return_t = std::optional<load_file_span>;

auto load_file(const char *path, const char *ext, bool append_null_byte) noexcept
    -> load_file_return_t;

auto os_open_read(const char *path, void *buffer, std::uintmax_t file_size) noexcept -> bool;

} // namespace surge

#endif // SURGE_FILE_HPP