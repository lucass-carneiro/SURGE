#ifndef SURGE_FILES_HPP
#define SURGE_FILES_HPP

#include "options.hpp"

#include <array>
#include <optional>
#include <string>
#include <tuple>

#ifdef SURGE_DEBUG_MEMORY
#  include <gsl/gsl-lite.hpp>
#else
#  include <span>
#endif

namespace surge::files {

#ifdef SURGE_DEBUG_MEMORY
using load_file_span = gsl::span<std::byte>;
#else
using load_file_span = std::span<std::byte>;
#endif

using load_file_return_t = std::optional<load_file_span>;

auto load_file(const char *path, bool append_null_byte) noexcept -> load_file_return_t;
void free_file(load_file_span &file) noexcept;

auto validate_path(const char *path) noexcept -> bool;

} // namespace surge::files

#endif // SURGE_FILES_HPP