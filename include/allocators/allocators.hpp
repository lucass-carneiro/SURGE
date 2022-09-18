#ifndef SURGE_ALLOCATORS_HPP
#define SURGE_ALLOCATORS_HPP

#include "log.hpp"

#include <cstddef>

namespace surge {

/**
 * @brief Determines if a number is a power of two, using bit operations.
 *
 * @param x The number to test
 * @return true If the number is a power of 2.
 * @return false It the numbe is not a power of 2.
 */
[[nodiscard]] constexpr inline auto is_pow_2(std::size_t x) -> bool { return (x & (x - 1)) == 0; }

/**
 * @brief Modifies an allocation size to be aligned with the specified
 * alignment
 *
 * @param intended_size The size one wishes to allocate.
 * @param alignment The alignment of the allocation.
 * @return std::size_t The aligned allocation size.
 */
[[nodiscard]] constexpr inline auto align_alloc_size(std::size_t intended_size,
                                                     std::size_t alignment
                                                     = alignof(std::max_align_t)) -> std::size_t {

  std::size_t modulo{0};

  if (is_pow_2(alignment)) {
    modulo = intended_size & (alignment - 1);
  } else {
    modulo = intended_size % alignment;
  }

  return intended_size + (alignment - modulo);
}

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

/**
 * @brief Adapts a base_allocator to work with stl containers
 *
 */
template <typename T> class stl_allocator {
public:
  using value_type = T;

  stl_allocator(base_allocator &sa) noexcept : allocator{sa} {}

  template <class U> constexpr stl_allocator(const stl_allocator<U> &) noexcept {}

  [[nodiscard]] auto allocate(std::size_t n) noexcept -> T * {
    const std::size_t intended_size = n * sizeof(T);
    constexpr const std::size_t alignment
        = alignof(T) < sizeof(void *) ? sizeof(void *) : alignof(T);
    const std::size_t actual_size = align_alloc_size(intended_size, alignment);

    return static_cast<T *>(allocator.aligned_alloc(alignment, actual_size));
  }

  void deallocate(T *p, std::size_t) noexcept { allocator.free(static_cast<void *>(p)); }

private:
  base_allocator &allocator;
};

} // namespace surge

#endif // SURGE_ALLOCATORS_HPP