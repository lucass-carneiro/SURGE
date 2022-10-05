#ifndef SURGE_LINEAR_ARENA_ALLOCATOR
#define SURGE_LINEAR_ARENA_ALLOCATOR

#include "base_allocator.hpp"
#include "log.hpp"
#include "options.hpp"

#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>

namespace surge {

/**
 * @brief Saves the state of the allocator for use in rewinding
 *
 */
struct linear_arena_allocator_state {
  std::size_t allocation_counter{0};
  std::size_t free_index{0};
};

/**
 * @brief A linear arena allocator.
 *
 * The linear arena allocator works by allocating a single contiguous block of
 * memory and dispensing memory from that block each time an allocation is
 * performed. There is no way to free individual objects and reuse a space
 * within the memory block. Instead, the arena can be resset and all of the
 * memory allocated in it's internal block will be freed at once.
 *
 * The arena requires an internal allocator type, that is responsible for
 * allocating it's internal buffer.
 *
 * @tparam parent_allocator_t The parent allocator for the arena, responsible
 * for allocating it's internal memory block.
 */
class linear_arena_allocator final : public base_allocator {
public:
  linear_arena_allocator() noexcept
      : arena_buffer(nullptr, [&](void *ptr) { parent_allocator->free(ptr); }) {}

  ~linear_arena_allocator() noexcept final;

  void init(base_allocator *pa, std::size_t capacity, const char *debug_name) noexcept;

  linear_arena_allocator(const linear_arena_allocator &) = delete;
  linear_arena_allocator(linear_arena_allocator &&) = delete;

  auto operator=(const linear_arena_allocator &) -> linear_arena_allocator & = delete;
  auto operator=(linear_arena_allocator &&) -> linear_arena_allocator & = delete;

  /**
   * @brief Allocate bytes of uninitialized storage with the specified
   * alignment requirements
   *
   * @note Passing a size which is not an integral multiple of alignment or an
   * alignment which is not valid or not supported by the implementation
   * causes the function to fail and return a null pointer.
   *
   * @param alignment Specifies the alignment. Must be a power of 2 and a
   * multiple of sizeof(void*).
   *
   * @param size Number of bytes to allocate. Must be an non-zero integral
   * multiple of alignment.
   *
   * @return On success, returns the pointer to the beginning of newly
   * allocated memory.
   *
   * @return On failure, returns a null pointer.
   */
  [[nodiscard]] auto aligned_alloc(std::size_t alignment, std::size_t size) noexcept
      -> void * final;

  /**
   * @brief Allocates bytes of uninitialized storage within the arena.
   *
   * @param size The number of bytes to allocate.
   *
   * @return A pointer to the lowest (first) byte in the allocated memory
   * block that is suitably aligned for any scalar type (at least as strictly
   * as std::max_align_t).
   *
   * @return nullptr if the size is zero or if the function fails to allocate.
   */
  [[nodiscard]] auto malloc(std::size_t size) noexcept -> void * final;

  /**
   * @brief Allocates memory for an array of objects and initializes it to all
   * bits zero.
   *
   * @note Due to the alignment requirements, the number of allocated bytes is
   * not necessarily equal to num*size.
   *
   * @param num Number of objects to allocate.
   *
   * @param size Size of each object to allocate.
   *
   * @return On success, returns the pointer to the beginning of newly
   * allocated memory.
   *
   * @return On failure, returns a null pointer
   */
  [[nodiscard]] auto calloc(std::size_t num, std::size_t size) noexcept -> void * final;

  /**
   * @brief Reallocates the given area of memory. Given that the arena is
   * unable to free a single allocation, in practice, this function simply
   * allocates a new block with the requested size within the arena.
   *
   * @param ptr Pointer to the memore area to be reallocated.
   *
   * @param new_size New size of the memory block.
   *
   * @return On success, returns a pointer to the beginning of newly allocated
   * memory.
   *
   * @return On failure, returns a null pointer.
   */
  [[nodiscard]] auto realloc(void *ptr, std::size_t new_size) noexcept -> void * final;

  /**
   * @brief Releases previoslly allocated memory. Since the arena allocator
   * cannot free individual objects, this function does if the flag
   * SURGE_DEBUG_MEMORY is not set. If it is, it decreases the allocation
   * counter.
   *
   * @param ptr Pointer to the memory to deallocate.
   */
  void free(void *ptr) noexcept final;

  /**
   * @brief Resets the arena to it's initial configuration. This allows it to
   * be reused for new allocations. This also means that any previously
   * existing pointers are invalidated after this call.
   */
  void reset() noexcept;

  /**
   * @brief Saves the state of the allocator at the point of invocation
   *
   */
  [[nodiscard]] auto save() const noexcept -> linear_arena_allocator_state;

  /**
   * @brief Restores the allocator to the saved point
   *
   */
  void restore(const linear_arena_allocator_state &) noexcept;

#ifdef SURGE_DEBUG_MEMORY
  /**
   * Returns the debug name of the arena.
   * @return The debug name of the arena.
   */
  [[nodiscard]] inline auto get_debug_name() const noexcept -> const char * {
    return allocator_debug_name;
  }
#endif

  /**
   * @brief Get the requested arena capacity
   *
   * @return The requested arena capacity
   */
  [[nodiscard]] inline auto get_requested_capacity() const noexcept -> std::size_t {
    return requested_arena_capacity;
  }

  /**
   * @brief Get the actual(after alignment) arena capacity
   *
   * @return The actual arena capacity after alignment
   */
  [[nodiscard]] inline auto get_actual_capacity() const noexcept -> std::size_t {
    return actual_arena_capacity;
  }

private:
  /**
   * The parent allocator to use for this arena.
   */
  base_allocator *parent_allocator{nullptr};

  /**
   * @brief The user requested total memory the arena can provide.
   */
  std::size_t requested_arena_capacity{0};

  /**
   * @brief The actual amount of memory the arena can provide.
   */
  std::size_t actual_arena_capacity{0};

#ifdef SURGE_DEBUG_MEMORY
  const char *allocator_debug_name{"linear arena allocator"};
  std::size_t allocation_counter{0};
#endif

  /**
   * Index to the first free adress in the underlying memory buffer
   */
  std::size_t free_index{0};

  /**
   * Pointer to the underlying memory buffer that is given to the callers.
   */
  std::unique_ptr<std::byte, std::function<void(void *)>> arena_buffer;
};

} // namespace surge

#endif // SURGE_LINEAR_ARENA_ALLOCATOR