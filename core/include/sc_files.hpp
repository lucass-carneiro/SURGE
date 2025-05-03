#ifndef SURGE_CORE_FILES_HPP
#define SURGE_CORE_FILES_HPP

#include "sc_allocators.hpp"
#include "sc_container_types.hpp"
#include "sc_error_types.hpp"

namespace surge::files {

auto is_path_valid(const char *path) -> bool;

auto as_bytes(const char *path,
              bool append_null_byte = false) -> Result<containers::mimalloc::Vector<std::byte>>;

// auto load_file(const char *path, bool append_null_byte) -> file;

} // namespace surge::files

#endif // SURGE_CORE_FILES_HPP