#ifndef SURGE_STL_ALLOCATOR_HPP
#define SURGE_STL_ALLOCATOR_HPP

#include "allocator_utils.hpp"

namespace surge {

/**
 * @brief Adapts a base_allocator to work with stl containers
 *
 */
template <typename T, typename surge_alloc_t> class stl_allocator {
public:
  using value_type = T;

  stl_allocator(surge_alloc_t *sa) noexcept : allocator{sa} {}

  template <class U, class V> constexpr stl_allocator(const stl_allocator<U, V> &) noexcept {}

  [[nodiscard]] inline auto allocate(std::size_t n) noexcept -> T * {
    const std::size_t intended_size = n * sizeof(T);
    constexpr const std::size_t alignment
        = alignof(T) < sizeof(void *) ? sizeof(void *) : alignof(T);
    const std::size_t actual_size = align_alloc_size(intended_size, alignment);

    return static_cast<T *>(allocator->aligned_alloc(alignment, actual_size));
  }

  inline void deallocate(T *p, std::size_t) noexcept { allocator->free(static_cast<void *>(p)); }

private:
  surge_alloc_t *allocator{nullptr};
};

} // namespace surge

#endif // SURGE_STL_ALLOCATOR_HPP