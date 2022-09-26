#ifndef SURGE_BASE_ALLOCATOR_HPP
#define SURGE_BASE_ALLOCATOR_HPP

#include "allocator_utils.hpp"
#include "log.hpp"

namespace surge {

class base_allocator {
public:
  [[nodiscard]] virtual auto malloc(std::size_t) noexcept -> void * = 0;

  [[nodiscard]] virtual auto aligned_alloc(std::size_t, std::size_t) noexcept -> void * = 0;

  [[nodiscard]] virtual auto calloc(std::size_t, std::size_t) noexcept -> void * = 0;

  [[nodiscard]] virtual auto realloc(void *ptr, std::size_t new_size) noexcept -> void * = 0;

  virtual void free(void *ptr) noexcept = 0;

  base_allocator() = default;
  base_allocator(const base_allocator &) = delete;
  base_allocator(base_allocator &&) = default;

  auto operator=(const base_allocator &) -> base_allocator & = delete;
  auto operator=(base_allocator &&) -> base_allocator & = default;

  virtual ~base_allocator() = default;
};

} // namespace surge

#endif // SURGE_BASE_ALLOCATOR_HPP