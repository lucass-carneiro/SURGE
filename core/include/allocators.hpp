#ifndef SURGE_CORE_ALLOCATORS_HPP
#define SURGE_CORE_ALLOCATORS_HPP

#include "integer_types.hpp"

#include <exception>
#include <limits>
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

template <class T> struct cpp_allocator {
  using value_type = T;

  cpp_allocator() = default;

  template <class U> constexpr cpp_allocator(const cpp_allocator<U> &) noexcept {}

  [[nodiscard]] auto allocate(std::size_t n) -> T * {
    if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
      throw std::bad_array_new_length();

    if (auto p = static_cast<T *>(surge::allocators::mimalloc::malloc(n * sizeof(T)))) {
      return p;
    }

    throw std::bad_alloc();
  }

  void deallocate(T *p, std::size_t) noexcept { surge::allocators::mimalloc::free(p); }
};

template <class T, class U>
auto operator==(const cpp_allocator<T> &, const cpp_allocator<U> &) -> bool {
  return true;
}

template <class T, class U>
auto operator!=(const cpp_allocator<T> &, const cpp_allocator<U> &) -> bool {
  return false;
}

} // namespace surge::allocators::mimalloc

#endif // SURGE_CORE_ALLOCATORS_HPP