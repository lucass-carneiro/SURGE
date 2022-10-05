#ifndef SURGE_IMAGE_LOADER_HPP
#define SURGE_IMAGE_LOADER_HPP

#include "allocators/allocators.hpp"
#include "file.hpp"
#include "stb/stb_image.hpp"

#include <filesystem>
#include <optional>

namespace surge {

template <surge_allocator alloc_t>
inline auto load_image(alloc_t *allocator, const std::filesystem::path &p, const char *ext) noexcept
    -> stbi_uc * {

  glog<log_event::message>("Loading image file {}", p.c_str());

  auto file{load_file(allocator, p, ext)};
  if (!file) {
    glog<log_event::error>("Unable to load image file {}", p.c_str());
    return nullptr;
  }

  int x{0}, y{0}, channels_in_file{0};
  auto img_data{stbi_load_from_memory(
      allocator, static_cast<stbi_uc *>(static_cast<void *>(file.value().data())),
      file.value().size(), &x, &y, &channels_in_file, 0)};

  if (img_data == nullptr) {
    glog<log_event::error>("Unable to load image file {} due to stbi error: {}", p.c_str(),
                           stbi_failure_reason());
    allocator->free(file.value().data());
    return nullptr;
  }

  allocator->free(file.value().data());
  return img_data;
}

} // namespace surge

#endif // SURGE_IMAGE_LOADER_HPP