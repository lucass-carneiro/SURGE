#ifndef SURGE_IMAGE_LOADER_HPP
#define SURGE_IMAGE_LOADER_HPP

#include "stb/stb_image.hpp"

#include <filesystem>
#include <optional>

namespace surge {

struct image {
  int width{0};
  int height{0};
  int channels_in_file{0};
  stbi_uc *data{nullptr};
};

auto load_image(const std::filesystem::path &p, const char *ext) noexcept -> std::optional<image>;

} // namespace surge

#endif // SURGE_IMAGE_LOADER_HPP