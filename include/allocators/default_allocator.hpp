#ifndef SURGE_DEFAULT_ALLOCATOR_HPP
#define SURGE_DEFAULT_ALLOCATOR_HPP

#include "base_allocator.hpp"
#include "options.hpp"

namespace surge {

class default_allocator final : public base_allocator {
public:
  default_allocator() noexcept = default;
  ~default_allocator() noexcept final = default;

  void init(const char *name) noexcept;

  default_allocator(const default_allocator &) noexcept = delete;
  default_allocator(default_allocator &&) noexcept = default;

  auto operator=(default_allocator) noexcept -> default_allocator & = delete;
  auto operator=(const default_allocator &) noexcept -> default_allocator & = delete;
  auto operator=(default_allocator &&) noexcept -> default_allocator & = default;

  [[nodiscard]] auto malloc(std::size_t size) noexcept -> void * final;

  [[nodiscard]] auto aligned_alloc(std::size_t alignment, std::size_t size) noexcept
      -> void * final;

  [[nodiscard]] auto calloc(std::size_t num, std::size_t size) noexcept -> void * final;

  [[nodiscard]] auto realloc(void *ptr, std::size_t new_size) noexcept -> void * final;

  void free(void *ptr) noexcept final;

#ifdef SURGE_DEBUG_MEMORY
  const char *allocator_debug_name{"Default allocator"};
#endif
};

} // namespace surge

#endif // SURGE_DEFAULT_ALLOCATOR_HPP