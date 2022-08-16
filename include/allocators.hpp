#ifndef SURGE_ALLOCATORS_HPP
#define SURGE_ALLOCATORS_HPP

#include <concepts>
#include <cstddef>
#include <cstdlib>

namespace surge {

template <typename T>
concept surge_allocator = requires(T a) {
  { a.malloc(std::size_t{0}) } -> std::same_as<void *>;
  { a.aligned_alloc(std::size_t{0}, std::size_t{0}) } -> std::same_as<void *>;
  { a.calloc(std::size_t{0}, std::size_t{0}) } -> std::same_as<void *>;
  { a.realloc(nullptr, std::size_t{0}) } -> std::same_as<void *>;
  { a.free(nullptr) } -> std::same_as<void>;
};

class default_allocator {
public:
  default_allocator() = default;
  default_allocator(const default_allocator &) = delete;
  default_allocator(default_allocator &&) = delete;

  auto operator=(default_allocator) -> default_allocator & = delete;
  auto operator=(const default_allocator &) -> default_allocator & = delete;
  auto operator=(default_allocator &&) -> default_allocator & = delete;

  ~default_allocator() = default;

  [[nodiscard]] inline auto malloc(std::size_t size) const noexcept -> void * {
    // NOLINTNEXTLINE
    return std::malloc(size);
  }

  [[nodiscard]] inline auto aligned_alloc(std::size_t alignment,
                                          std::size_t size) const noexcept
      -> void * {
    // NOLINTNEXTLINE
    return std::aligned_alloc(alignment, size);
  }

  [[nodiscard]] inline auto calloc(std::size_t num,
                                   std::size_t size) const noexcept {
    // NOLINTNEXTLINE
    return std::calloc(num, size);
  }

  [[nodiscard]] inline auto realloc(void *ptr,
                                    std::size_t new_size) const noexcept {
    // NOLINTNEXTLINE
    return std::realloc(ptr, new_size);
  }

  inline void free(void *ptr) const noexcept {
    // NOLINTNEXTLINE
    std::free(ptr);
  }
};

} // namespace surge

#endif // SURGE_ALLOCATORS_HPP