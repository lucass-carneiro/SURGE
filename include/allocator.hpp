#ifndef SURGE_ALLOCATOR_HPP
#define SURGE_ALLOCATOR_HPP

#include <mimalloc.h>

namespace surge {

void init_mimalloc() noexcept;

// See https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2271.html#eastl_allocator
class eastl_allocator {
public:
  eastl_allocator(const char * = nullptr) noexcept {}
  eastl_allocator(const eastl_allocator &a, const char *) noexcept : eastl_allocator(a) {}

  [[nodiscard]] auto allocate(size_t n, int flags = 0) const noexcept -> void *;
  [[nodiscard]] auto allocate(size_t n, size_t alignment, size_t, int flags = 0) const noexcept
      -> void *;
  void deallocate(void *p, size_t n) const noexcept;

  [[nodiscard]] auto get_name() const noexcept -> const char *;
  void set_name(const char *);
};

[[nodiscard]] auto operator==(const eastl_allocator &, const eastl_allocator &) noexcept -> bool;
[[nodiscard]] auto operator!=(const eastl_allocator &, const eastl_allocator &) noexcept -> bool;

} // namespace surge

#endif // SURGE_ALLOCATOR_HPP