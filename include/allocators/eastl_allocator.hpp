#ifndef SURGE_EASTL_ALLOCATOR_HPP
#define SURGE_EASTL_ALLOCATOR_HPP

#include "allocators.hpp"
#include "options.hpp"

#include <EASTL/allocator.h>

namespace surge {

class eastl_allocator {
public:
  eastl_allocator(base_allocator *alloc) noexcept : allocator{alloc} {}

  [[nodiscard]] auto allocate(std::size_t n, int) noexcept -> void * {
    return allocator->malloc(n);
  }

  [[nodiscard]] auto allocate(std::size_t n, std::size_t alignment, std::size_t, int) noexcept
      -> void * {
    return allocator->aligned_alloc(alignment, n);
  }

  void deallocate(void *p, std::size_t) noexcept { allocator->free(p); }

  [[nodiscard]] auto get_name() const noexcept -> const char * { return "default eastl allocator"; }

  void set_name(const char *) const noexcept {}

private:
  base_allocator *allocator{nullptr};
};

[[nodiscard]] inline auto operator==(const eastl::allocator &, const eastl::allocator &) noexcept
    -> bool {
  return false;
}

[[nodiscard]] inline auto operator!=(const eastl::allocator &, const eastl::allocator &) noexcept
    -> bool {
  return true;
}

} // namespace surge

#endif // SURGE_EASTL_ALLOCATOR_HPP