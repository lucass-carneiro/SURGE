#ifndef SURGE_FILES_HPP
#define SURGE_FILES_HPP

#include "container_types.hpp"
#include "error_types.hpp"

#include <tl/expected.hpp>

namespace surge::files {

using file_size_t = std::uintmax_t;
using file_data_t = vector<std::byte>;
using file = tl::expected<file_data_t, error>;

auto validate_path(const char *path) noexcept -> bool;

auto load_file(const char *path, bool append_null_byte) noexcept -> file;

} // namespace surge::files

#endif // SURGE_FILES_HPP