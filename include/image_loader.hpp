#ifndef SURGE_IMAGE_LOADER_HPP
#define SURGE_IMAGE_LOADER_HPP

#include "allocators/global_allocators.hpp"

#include <filesystem>
#include <png.h>
#include <vector>

namespace surge {

class global_image_loader {
public:
  static inline auto get() -> global_image_loader & {
    static global_image_loader instance;
    return instance;
  }

  global_image_loader(const global_image_loader &) = delete;
  global_image_loader(global_image_loader &&) = delete;

  auto operator=(global_image_loader) -> global_image_loader & = delete;
  auto operator=(const global_image_loader &) -> global_image_loader & = delete;
  auto operator=(global_image_loader &&) -> global_image_loader & = delete;

  static const std::size_t subsystem_allocator_capacity;
  static const std::size_t persistent_allocator_capacity;
  static const std::size_t volatile_allocator_capacity;

  void load_persistent(const std::filesystem::path &);

private:
  global_image_loader() noexcept = default;
};

} // namespace surge

#endif // SURGE_IMAGE_LOADER_HPP