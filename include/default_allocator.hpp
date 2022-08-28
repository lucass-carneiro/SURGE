#ifndef SURGE_DEFAULT_ALLOCATOR_HPP
#define SURGE_DEFAULT_ALLOCATOR_HPP

#include "allocators.hpp"

#include <cstdlib>

namespace surge {

class default_allocator final : public surge_allocator {
public:
  default_allocator() = default;
  default_allocator(const default_allocator &) = delete;
  default_allocator(default_allocator &&) = default;

  auto operator=(default_allocator) -> default_allocator & = delete;
  auto operator=(const default_allocator &) -> default_allocator & = delete;
  auto operator=(default_allocator &&) -> default_allocator & = default;

  ~default_allocator() final = default;

  [[nodiscard]] inline auto malloc(std::size_t size) noexcept -> void * final {
    // NOLINTNEXTLINE
    return std::malloc(size);
  }

  [[nodiscard]] inline auto aligned_alloc(std::size_t alignment,
                                          std::size_t size) noexcept
      -> void * final {
    // NOLINTNEXTLINE
    return std::aligned_alloc(alignment, size);
  }

  [[nodiscard]] inline auto calloc(std::size_t num, std::size_t size) noexcept
      -> void * final {
    // NOLINTNEXTLINE
    return std::calloc(num, size);
  }

  [[nodiscard]] inline auto realloc(void *ptr, std::size_t new_size) noexcept
      -> void * final {
    // NOLINTNEXTLINE
    return std::realloc(ptr, new_size);
  }

  inline void free(void *ptr) noexcept final {
    // NOLINTNEXTLINE
    std::free(ptr);
  }
};

} // namespace surge

#endif // SURGE_DEFAULT_ALLOCATOR_HPP