#ifndef SURGE_FILES_HPP
#define SURGE_FILES_HPP

#include "allocators.hpp"
#include "options.hpp"

#include <EASTL/vector.h>
#include <tl/expected.hpp>

namespace surge::files {

enum class file_error { size_truncation, invalid_path, read_error, invalid_format, unknow_error };

#ifdef SURGE_SYSTEM_Windows
using file_size_t = unsigned int;
#else
using file_size_t = std::uintmax_t;
#endif

using file
    = tl::expected<eastl::vector<std::byte, surge::allocators::eastl::gp_allocator>, file_error>;

auto validate_path(const char *path) noexcept -> bool;
auto get_file_size(const char *path) noexcept -> tl::expected<file_size_t, file_error>;

auto load_file(const char *path, bool append_null_byte) noexcept -> file;

} // namespace surge::files

#endif // SURGE_FILES_HPP