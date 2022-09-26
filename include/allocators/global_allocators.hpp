#ifndef SURGE_GLOBAL_ALLOCATORS
#define SURGE_GLOBAL_ALLOCATORS

#include "default_allocator.hpp"
#include "linear_arena_allocator.hpp"
#include "options.hpp"
#include "stack_allocator.hpp"

#include <cstddef>

namespace surge {

class global_default_allocator {
public:
  static inline auto get() -> default_allocator & {
    static default_allocator alloc;
    return alloc;
  }

  global_default_allocator(const global_default_allocator &) = delete;
  global_default_allocator(global_default_allocator &&) = delete;

  auto operator=(global_default_allocator) -> global_default_allocator & = delete;

  auto operator=(const global_default_allocator &) -> global_default_allocator & = delete;

  auto operator=(global_default_allocator &&) -> global_default_allocator & = delete;

  ~global_default_allocator() = default;

private:
  global_default_allocator() = default;
};

class global_linear_arena_allocator {
public:
  static inline auto get() -> linear_arena_allocator & {
    static linear_arena_allocator alloc;
    return alloc;
  }

  global_linear_arena_allocator(const global_linear_arena_allocator &) = delete;
  global_linear_arena_allocator(global_linear_arena_allocator &&) = delete;

  auto operator=(global_linear_arena_allocator) -> global_linear_arena_allocator & = delete;

  auto operator=(const global_linear_arena_allocator &) -> global_linear_arena_allocator & = delete;

  auto operator=(global_linear_arena_allocator &&) -> global_linear_arena_allocator & = delete;

  ~global_linear_arena_allocator() = default;

private:
  global_linear_arena_allocator() = default;
};

class global_stack_allocator {
public:
  static inline auto get() -> stack_allocator & {
    static stack_allocator alloc;
    return alloc;
  }

  global_stack_allocator(const global_stack_allocator &) = delete;
  global_stack_allocator(global_stack_allocator &&) = delete;

  auto operator=(global_stack_allocator) -> global_stack_allocator & = delete;

  auto operator=(const global_stack_allocator &) -> global_stack_allocator & = delete;

  auto operator=(global_stack_allocator &&) -> global_stack_allocator & = delete;

  ~global_stack_allocator() = default;

private:
  global_stack_allocator() = default;
};

} // namespace surge

#endif // SURGE_GLOBAL_ALLOCATORS