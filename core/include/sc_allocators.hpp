#ifndef SURGE_CORE_ALLOCATORS_HPP
#define SURGE_CORE_ALLOCATORS_HPP

#include "sc_integer_types.hpp"

#include <cstddef>
#include <exception>
#include <limits>
#include <type_traits>
#include <vector>

namespace surge::allocators::mimalloc {

void init();

auto malloc(usize size) -> void *;
auto realloc(void *p, usize newsize) -> void *;
auto calloc(usize count, usize size) -> void *;
void free(void *p);

auto aligned_alloc(usize size, usize alignment) -> void *;
auto aligned_realloc(void *p, usize newsize, usize alignment) -> void *;
void aligned_free(void *p, usize alignment);

template <class T> struct cpp_allocator {
  using value_type = T;

  cpp_allocator() = default;

  template <class U> constexpr cpp_allocator(const cpp_allocator<U> &) {}

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

class arena {
private:
  using byte_t = std::byte;
  std::vector<byte_t, cpp_allocator<byte_t>> data{};

  usize capacity{0};
  usize offset{0};

public:
  arena(usize capacity);
  auto allocate(usize size, usize alignment) -> void *;
  void reset();
  auto size() const -> usize;
};

} // namespace surge::allocators::mimalloc

#endif // SURGE_CORE_ALLOCATORS_HPP