#ifndef SURGE_CORE_ALLOCATORS_HPP
#define SURGE_CORE_ALLOCATORS_HPP

#include "sc_error_types.hpp"
#include "sc_integer_types.hpp"

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

template <typename T> struct STLAllocator {
  using value_type = T;

  STLAllocator() = default;

  [[nodiscard]] auto allocate(std::size_t n) -> T * {
    if (n > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
      throw std::bad_array_new_length();
    }

    if (auto p = static_cast<T *>(surge::allocators::mimalloc::malloc(n * sizeof(T)))) {
      return p;
    }

    throw std::bad_alloc();
  }

  void deallocate(T *p, std::size_t) noexcept { surge::allocators::mimalloc::free(p); }
};

template <class T, class U> inline auto operator==(const STLAllocator<T> &, const STLAllocator<U> &)
    -> bool {
  return true;
}

template <class T, class U> inline auto operator!=(const STLAllocator<T> &, const STLAllocator<U> &)
    -> bool {
  return false;
}

} // namespace mimalloc

namespace scoped {

enum class Lifetimes { Program, Module, Frame };

auto init() -> Result<void>;
void destroy();

auto malloc(const Lifetimes &lifetime, usize size, usize alignment) -> Result<void *>;
void reset(const Lifetimes &lifetime);

template <typename T> class STLAllocator {
private:
  Lifetimes lifetime{};

public:
  using value_type = T;

  STLAllocator(Lifetimes l) : lifetime{l} {}

  [[nodiscard]] auto allocate(std::size_t n) -> T * {
    if (n > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
      throw std::bad_array_new_length();
    }

    const auto result{scoped::malloc(lifetime, n * sizeof(T), alignof(T))};

    if (!result) {
      throw std::bad_alloc();
    }

    return static_cast<T *>(*result);
  }

  void deallocate(T *, std::size_t) noexcept {}
};

template <class T, class U> inline auto operator==(const STLAllocator<T> &, const STLAllocator<U> &)
    -> bool {
  return true;
}

template <class T, class U> inline auto operator!=(const STLAllocator<T> &, const STLAllocator<U> &)
    -> bool {
  return false;
}

} // namespace scoped
} // namespace surge::allocators

#endif // SURGE_CORE_ALLOCATORS_HPP