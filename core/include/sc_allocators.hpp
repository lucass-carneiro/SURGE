#ifndef SURGE_CORE_ALLOCATORS_HPP
#define SURGE_CORE_ALLOCATORS_HPP

#include "sc_error_types.hpp"
#include "sc_integer_types.hpp"

#include <mutex>

namespace surge::allocators {

namespace mimalloc {

void init();

auto malloc(usize size) -> void *;
void free(void *p);

auto aligned_alloc(usize size, usize alignment) -> void *;
auto aligned_realloc(void *p, usize newsize, usize alignment) -> void *;

template <typename T> class STLAllocator {
public:
  using value_type = T;

  STLAllocator() noexcept = default;

  template <typename U> STLAllocator(const STLAllocator<U> &) noexcept {}

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

  // Rebind allocator to another type
  template <typename U> struct rebind {
    using other = STLAllocator<U>;
  };
};

// Required for allocator equality comparisons
template <typename T, typename U>
bool operator==(const STLAllocator<T> &, const STLAllocator<U> &) noexcept {
  return true;
}

template <typename T, typename U>
bool operator!=(const STLAllocator<T> &, const STLAllocator<U> &) noexcept {
  return false;
}

} // namespace mimalloc

class DynamicArena {
private:
  usize current_capacity{0};
  usize current_offset{0};
  const char *arena_name{nullptr};
  std::byte *memory_data{nullptr};
  std::mutex mutex{};

  [[nodiscard]] auto is_pow_2(surge::usize x) const noexcept -> bool;

public:
  DynamicArena(usize initial_size, const char *name = "Dynamic Arena") noexcept;
  ~DynamicArena() noexcept;

  DynamicArena(DynamicArena &) = delete;
  auto operator=(const DynamicArena &) -> DynamicArena & = delete;

  auto allocate(std::size_t size, std::size_t alignment) -> void *;
  void reset() noexcept;
};

template <typename T> class DynamicArenaSTLAllocator {
private:
  DynamicArena *arena{nullptr};

public:
  using value_type = T;

  explicit DynamicArenaSTLAllocator(DynamicArena &arena) noexcept : arena{&arena} {}

  template <typename U> DynamicArenaSTLAllocator(const DynamicArenaSTLAllocator<U> &other) noexcept
      : arena{other.arena} {}

  T *allocate(std::size_t n) {
    return static_cast<T *>(arena->allocate(n * sizeof(T), alignof(T)));
  }

  void deallocate(T *, std::size_t) noexcept {}

  template <typename U> struct rebind {
    using other = DynamicArenaSTLAllocator<U>;
  };

  template <typename U> friend class DynamicArenaSTLAllocator;

  bool operator==(const DynamicArenaSTLAllocator &other) const noexcept {
    return arena == other.arena;
  }

  bool operator!=(const DynamicArenaSTLAllocator &other) const noexcept {
    return !(*this == other);
  }
};

} // namespace surge::allocators

#endif // SURGE_CORE_ALLOCATORS_HPP