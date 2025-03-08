#ifndef SURGE_CORE_ALLOCATORS_HPP
#define SURGE_CORE_ALLOCATORS_HPP

#include "sc_error_types.hpp"
#include "sc_integer_types.hpp"

#include <cstddef>
#include <exception>
#include <limits>
#include <tl/expected.hpp>
#include <type_traits>
#include <vector>

namespace surge::allocators {

namespace mimalloc {

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

template <class T, class U> auto operator==(const cpp_allocator<T> &, const cpp_allocator<U> &)
    -> bool {
  return true;
}

template <class T, class U> auto operator!=(const cpp_allocator<T> &, const cpp_allocator<U> &)
    -> bool {
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

template <class T> class arena_cpp_allocator {
private:
  arena *arena_allocator;

public:
  using value_type = T;

  arena_cpp_allocator(arena *a) : arena_allocator{a} {}

  [[nodiscard]] auto allocate(std::size_t n) -> T * {
    if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
      throw std::bad_array_new_length();

    if (auto p = static_cast<T *>(arena_allocator->allocate(n * sizeof(T), alignof(T)))) {
      return p;
    }

    throw std::bad_alloc();
  }

  void deallocate(T *, std::size_t) noexcept {}
};

} // namespace mimalloc

namespace program_scope {

auto init() -> tl::expected<void, surge::error>;
void destroy();
auto malloc(usize size, usize alignment) -> tl::expected<void *, error>;

template <typename T> class cpp_allocator {
public:
  using value_type = T;

  cpp_allocator() = default;

  [[nodiscard]] inline auto allocate(std::size_t n) -> T * {
    if (n > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
      throw std::bad_array_new_length();
    }

    if (auto p = static_cast<T *>(program_scope::malloc(n * sizeof(T), alignof(T)))) {
      return p;
    }

    throw std::bad_alloc();
  }

  void deallocate(T *, std::size_t) noexcept {}
};

} // namespace program_scope

} // namespace surge::allocators

#endif // SURGE_CORE_ALLOCATORS_HPP