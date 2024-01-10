#ifndef SURGE_ALLOCATORS_HPP
#define SURGE_ALLOCATORS_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace surge::allocators::mimalloc {

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

} // namespace surge::allocators::mimalloc

#endif // SURGE_ALLOCATORS_HPP