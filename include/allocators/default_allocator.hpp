#ifndef SURGE_DEFAULT_ALLOCATOR_HPP
#define SURGE_DEFAULT_ALLOCATOR_HPP

#include "allocators.hpp"
#include "options.hpp"

namespace surge {

class default_allocator final : public base_allocator {
public:
#ifdef SURGE_DEBUG_MEMORY
  default_allocator(const char *debug_name) : allocator_debug_name(debug_name) {}
#else
  default_allocator() = default;
#endif

  default_allocator(const default_allocator &) = delete;
  default_allocator(default_allocator &&) = default;

  auto operator=(default_allocator) -> default_allocator & = delete;
  auto operator=(const default_allocator &) -> default_allocator & = delete;
  auto operator=(default_allocator &&) -> default_allocator & = default;

#ifdef SURGE_DEBUG_MEMORY
  ~default_allocator() noexcept final;
#else
  ~default_allocator() final = default;
#endif

  [[nodiscard]] auto malloc(std::size_t size) noexcept -> void * final;

  [[nodiscard]] auto aligned_alloc(std::size_t alignment, std::size_t size) noexcept
      -> void * final;

  [[nodiscard]] auto calloc(std::size_t num, std::size_t size) noexcept -> void * final;

  [[nodiscard]] auto realloc(void *ptr, std::size_t new_size) noexcept -> void * final;

  void free(void *ptr) noexcept final;

#ifdef SURGE_DEBUG_MEMORY
private:
  const char *allocator_debug_name;
#endif
};

} // namespace surge

#endif // SURGE_DEFAULT_ALLOCATOR_HPP