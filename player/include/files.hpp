#ifndef SURGE_FILES_HPP
#define SURGE_FILES_HPP

#include "allocators.hpp"
#include "options.hpp"

#include <foonathan/memory/std_allocator.hpp>
#include <tl/expected.hpp>
#include <vector>

namespace surge::files {

enum class file_error { invalid_path, read_error, invalid_format, unknow_error };

using file_size_t = std::uintmax_t;

using file_data_t
    = std::vector<std::byte,
                  foonathan::memory::std_allocator<std::byte, allocators::mimalloc::fnm_allocator>>;
using file = tl::expected<file_data_t, file_error>;

auto validate_path(const char *path) noexcept -> bool;

auto load_file(const char *path, bool append_null_byte) noexcept -> file;

} // namespace surge::files

#endif // SURGE_FILES_HPP