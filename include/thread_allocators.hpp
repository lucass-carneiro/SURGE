#ifndef SURGE_THREAD_ARENAS_HPP
#define SURGE_THREAD_ARENAS_HPP

#include "allocators/global_allocators.hpp"
#include "allocators/stl_allocator.hpp"

#include <functional>
#include <memory>
#include <vector>

namespace surge {

class global_thread_allocators {
public:
  using stack_allocator_ptr = std::unique_ptr<stack_allocator, std::function<void(void *)>>;
  using stl_allocator_t = stl_allocator<stack_allocator_ptr>;
  using alloc_vec_t = std::vector<stack_allocator_ptr, stl_allocator_t>;

  static inline auto get() -> global_thread_allocators & {
    static global_thread_allocators arenas;
    return arenas;
  }

  void init(unsigned int nt, long mpt) noexcept;

  auto at(std::size_t i) noexcept -> stack_allocator_ptr &;

  [[nodiscard]] inline auto get_num_threads() const noexcept -> unsigned int { return num_threads; }
  [[nodiscard]] inline auto get_num_workers() const noexcept -> unsigned int {
    return num_threads - 1;
  }

  ~global_thread_allocators() noexcept;

  global_thread_allocators(const global_thread_allocators &) = delete;
  global_thread_allocators(global_thread_allocators &&) = delete;

  auto operator=(global_thread_allocators) -> global_thread_allocators & = delete;

  auto operator=(const global_thread_allocators &) -> global_thread_allocators & = delete;

  auto operator=(global_thread_allocators &&) -> global_thread_allocators & = delete;

private:
  global_thread_allocators() = default;

  unsigned int num_threads{0};
  long memory_per_thread{0};

  linear_arena_allocator *parent_allocator{&global_linear_arena_allocator::get()};
  stl_allocator_t parent_stl_allocator{parent_allocator};

  alloc_vec_t allocator_array{parent_stl_allocator};
};

} // namespace surge

#endif // SURGE_THREAD_ARENAS_HPP