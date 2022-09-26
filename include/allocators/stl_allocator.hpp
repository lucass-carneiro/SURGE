#ifndef SURGE_STL_ALLOCATOR_HPP
#define SURGE_STL_ALLOCATOR_HPP

#include "base_allocator.hpp"

namespace surge {

/**
 * @brief Adapts a base_allocator to work with stl containers
 *
 */
template <typename T> class stl_allocator {
public:
  using value_type = T;

  stl_allocator(base_allocator *sa) noexcept : allocator{sa} {}

  template <class U> constexpr stl_allocator(const stl_allocator<U> &) noexcept {}

  [[nodiscard]] inline auto allocate(std::size_t n) noexcept -> T * {
    const std::size_t intended_size = n * sizeof(T);
    constexpr const std::size_t alignment
        = alignof(T) < sizeof(void *) ? sizeof(void *) : alignof(T);
    const std::size_t actual_size = align_alloc_size(intended_size, alignment);

    return static_cast<T *>(allocator->aligned_alloc(alignment, actual_size));
  }

  inline void deallocate(T *p, std::size_t) noexcept { allocator->free(static_cast<void *>(p)); }

private:
  base_allocator *allocator{nullptr};
};

} // namespace surge

#endif // SURGE_STL_ALLOCATOR_HPP