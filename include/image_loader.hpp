#ifndef SURGE_IMAGE_LOADER_HPP
#define SURGE_IMAGE_LOADER_HPP

#include "global_allocators.hpp"
#include "linear_arena_allocator.hpp"

#include <filesystem>
#include <vector>

namespace surge {

class global_image_loader {
public:
  static inline auto get() -> global_image_loader & {
    static global_image_loader instance;
    return instance;
  }

  ~global_image_loader() = default;

  global_image_loader(const global_image_loader &) = delete;
  global_image_loader(global_image_loader &&) = delete;

  auto operator=(global_image_loader) -> global_image_loader & = delete;
  auto operator=(const global_image_loader &) -> global_image_loader & = delete;
  auto operator=(global_image_loader &&) -> global_image_loader & = delete;

  static const std::size_t subsystem_allocator_capacity;
  static const std::size_t persistent_allocator_capacity;
  static const std::size_t volatile_allocator_capacity;

  [[nodiscard]] inline auto get_palloc() noexcept -> linear_arena_allocator & {
    return persistent_allocator;
  }

  [[nodiscard]] inline auto get_valloc() noexcept -> linear_arena_allocator & {
    return volatile_allocator;
  }

  void load_persistent(const std::filesystem::path &);

private:
  global_image_loader()
      : subsystem_allocator(global_linear_arena_allocator::get(), subsystem_allocator_capacity,
                            "Image loader subystem allocator"),
        persistent_allocator(subsystem_allocator, persistent_allocator_capacity,
                             "Image loader persistent allocator"),
        volatile_allocator(subsystem_allocator, volatile_allocator_capacity,
                           "Image loader volatile allocator"),
        persistent_image_data(stl_allocator<std::byte *>(persistent_allocator)),
        persistent_image_x(stl_allocator<int>(persistent_allocator)),
        persistent_image_y(stl_allocator<int>(persistent_allocator)),
        persistent_image_n(stl_allocator<int>(persistent_allocator)) {}

  linear_arena_allocator subsystem_allocator;
  linear_arena_allocator persistent_allocator;
  linear_arena_allocator volatile_allocator;

  std::vector<std::byte *, stl_allocator<std::byte *>> persistent_image_data;
  std::vector<int, stl_allocator<int>> persistent_image_x;
  std::vector<int, stl_allocator<int>> persistent_image_y;
  std::vector<int, stl_allocator<int>> persistent_image_n;
};

} // namespace surge

#endif // SURGE_IMAGE_LOADER_HPP