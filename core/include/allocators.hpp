#ifndef SURGE_CORE_ALLOCATORS_HPP
#define SURGE_CORE_ALLOCATORS_HPP

#include "integer_types.hpp"

#include <type_traits>

namespace surge::allocators::mimalloc {

void init() noexcept;

auto malloc(usize size) noexcept -> void *;
auto realloc(void *p, usize newsize) noexcept -> void *;
auto calloc(usize count, usize size) noexcept -> void *;
void free(void *p) noexcept;

auto aligned_alloc(usize size, usize alignment) -> void *;
auto aligned_realloc(void *p, usize newsize, usize alignment) noexcept -> void *;
void aligned_free(void *p, usize alignment) noexcept;

struct fnm_allocator {
  using is_stateful = std::false_type;

  auto allocate_node(usize size, usize alignment) -> void *;
  void deallocate_node(void *node, usize size, usize alignment) noexcept;
};

} // namespace surge::allocators::mimalloc

#endif // SURGE_CORE_ALLOCATORS_HPP