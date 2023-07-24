#ifndef SURGE_FILES_HPP
#define SURGE_FILES_HPP

#include <array>
#include <optional>
#include <string>
#include <tuple>

namespace surge::files {

auto validate_path(const char *path) noexcept -> bool;

} // namespace surge::files

#endif // SURGE_FILES_HPP