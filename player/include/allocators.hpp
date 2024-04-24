#ifndef SURGE_ALLOCATORS_HPP
#define SURGE_ALLOCATORS_HPP

#include "integer_types.hpp"

#include <type_traits>

namespace surge::allocators::mimalloc {

void init() noexcept;

auto malloc(usize size) noexcept -> void *;
auto realloc(void *p, usize newsize) noexcept -> void *;
auto calloc(usize count, usize size) noexcept -> void *;
void free(void *p) noexcept;

struct fnm_allocator {
  using is_stateful = std::false_type;

  auto allocate_node(usize size, usize alignment) -> void *;
  void deallocate_node(void *node, usize size, usize alignment) noexcept;
};

} // namespace surge::allocators::mimalloc

#endif // SURGE_ALLOCATORS_HPP