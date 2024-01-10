#ifndef SURGE_ALLOCATORS_HPP
#define SURGE_ALLOCATORS_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace surge::allocators {

namespace mimalloc {

void init() noexcept;

void free(void *p) noexcept;
auto malloc(size_t size) noexcept -> void *;
auto zalloc(size_t size) noexcept -> void *;
auto calloc(size_t count, size_t size) noexcept -> void *;
auto realloc(void *p, size_t newsize) noexcept -> void *;
auto recalloc(void *p, size_t count, size_t size) noexcept -> void *;
auto expand(void *p, size_t newsize) noexcept -> void *;
auto mallocn(size_t count, size_t size) noexcept -> void *;
auto reallocn(void *p, size_t count, size_t size) noexcept -> void *;
auto reallocf(void *p, size_t newsize) noexcept -> void *;
auto strdup(const char *s) noexcept -> char *;
auto strndup(const char *s, size_t n) noexcept -> char *;
auto realpath(const char *fname, char *resolved_name) noexcept -> char *;

struct fnm_allocator {
  using is_stateful = std::false_type;

  auto allocate_node(std::size_t size, std::size_t alignment) -> void *;
  void deallocate_node(void *node, std::size_t size, std::size_t alignment) noexcept;
};

} // namespace mimalloc

namespace eastl {

// See https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2271.html#eastl_allocator
class gp_allocator {
public:
  gp_allocator(const char * = nullptr) noexcept {}
  gp_allocator(const gp_allocator &a, const char *) noexcept : gp_allocator(a) {}

  [[nodiscard]] auto allocate(size_t n, int flags = 0) const noexcept -> void *;
  [[nodiscard]] auto allocate(size_t n, size_t alignment, size_t, int flags = 0) const noexcept
      -> void *;
  void deallocate(void *p, size_t n) const noexcept;

  [[nodiscard]] auto get_name() const noexcept -> const char *;
  void set_name(const char *);
};

[[nodiscard]] auto operator==(const gp_allocator &, const gp_allocator &) noexcept -> bool;
[[nodiscard]] auto operator!=(const gp_allocator &, const gp_allocator &) noexcept -> bool;

} // namespace eastl

} // namespace surge::allocators

#endif // SURGE_ALLOCATORS_HPP