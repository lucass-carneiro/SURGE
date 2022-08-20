#ifndef SURGE_ALLOCATORS_HPP
#define SURGE_ALLOCATORS_HPP

#include <cstddef>

namespace surge {

class surge_allocator {
public:
  [[nodiscard]] virtual auto malloc(std::size_t) noexcept -> void * = 0;

  [[nodiscard]] virtual auto aligned_alloc(std::size_t, std::size_t) noexcept
      -> void * = 0;

  [[nodiscard]] virtual auto calloc(std::size_t, std::size_t) noexcept
      -> void * = 0;

  [[nodiscard]] virtual auto realloc(void *ptr, std::size_t new_size) noexcept
      -> void * = 0;

  virtual void free(void *ptr) noexcept = 0;

  surge_allocator() = default;
  surge_allocator(const surge_allocator &) = delete;
  surge_allocator(surge_allocator &&) = delete;

  auto operator=(const surge_allocator &) -> surge_allocator & = delete;
  auto operator=(surge_allocator &&) -> surge_allocator & = delete;

  virtual ~surge_allocator() = default;
};

} // namespace surge

#endif // SURGE_ALLOCATORS_HPP