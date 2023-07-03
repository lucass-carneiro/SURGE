#ifndef SURGE_LOAD_TEXTURE_HPP
#define SURGE_LOAD_TEXTURE_HPP

#include "headers.hpp"

#include <filesystem>
#include <optional>

namespace surge {

[[nodiscard]] auto load_texture(const char *p, const char *ext) noexcept -> std::optional<GLuint>;

} // namespace surge

#endif // SURGE_LOAD_TEXTURE_HPP