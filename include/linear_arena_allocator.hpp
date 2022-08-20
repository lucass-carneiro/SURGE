#ifndef SURGE_LINEAR_ARENA_ALLOCATOR
#define SURGE_LINEAR_ARENA_ALLOCATOR

#include "allocators.hpp"
#include "log.hpp"
#include "options.hpp"

#include <gsl/gsl-lite.hpp>

#include <cstdint>
#include <cstring>
#include <vector>

namespace surge {

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
class linear_arena_allocator final : public surge_allocator {
public:
#ifdef SURGE_DEBUG_MEMORY
  /**
   * @brief Construct a new linear arena allocator object
   *
   * @param pa The parent allocator to use while allocating memory blocks.
   *
   * @param capacity The total size (in Bytes) that the arena can allocate.
   *
   * @param debug_name The debug name of this instance of the arena
   */
  linear_arena_allocator(surge_allocator &pa, std::size_t capacity,
                         const char *debug_name);
#else
  /**
   * @brief Construct a new linear arena allocator object
   *
   * @param pa The parent allocator to use while allocating memory blocks.
   *
   * @param capacity The total size (in Bytes) that the arena can allocate.
   */
  linear_arena_allocator(surge_allocator &pa, std::size_t capacity);
#endif

  /**
   * @brief Destroy the linear arena allocator object.
   *
   */
  ~linear_arena_allocator() final;

  linear_arena_allocator(const linear_arena_allocator &) = delete;
  linear_arena_allocator(linear_arena_allocator &&) = delete;

  auto operator=(const linear_arena_allocator &)
      -> linear_arena_allocator & = delete;
  auto operator=(linear_arena_allocator &&)
      -> linear_arena_allocator & = delete;

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
  [[nodiscard]] auto aligned_alloc(std::size_t alignment,
                                   std::size_t size) noexcept -> void * final;

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
  [[nodiscard]] auto calloc(std::size_t num, std::size_t size) noexcept
      -> void * final;

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
  [[nodiscard]] auto realloc(void *ptr, std::size_t new_size) noexcept
      -> void * final;

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
  void inline reset() noexcept {
    free_index = 0;
    allocation_counter = 0;
  }

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
  [[nodiscard]] inline auto get_requested_capacity() const noexcept
      -> std::size_t {
    return requested_arena_capacity;
  }

  /**
   * @brief Get the actual(after alignment) arena capacity
   *
   * @return The actual arena capacity after alignment
   */
  [[nodiscard]] inline auto get_actual_capacity() const noexcept
      -> std::size_t {
    return actual_arena_capacity;
  }

private:
  /**
   * The parent allocator to use for this arena.
   */
  surge_allocator &parent_allocator;

  /**
   * @brief The user requested total memory the arena can provide.
   */
  const std::size_t requested_arena_capacity;

  /**
   * @brief The default alignment
   */
  const std::size_t default_alignment{alignof(std::max_align_t)};

  /**
   * @brief The actual amount of memory the arena can provide.
   */
  const std::size_t actual_arena_capacity;

#ifdef SURGE_DEBUG_MEMORY
  const char *allocator_debug_name;
  std::size_t allocation_counter{0};
#endif

  /**
   * Index to the first free adress in the underlying memory buffer
   */
  std::size_t free_index{0};

  /**
   * Pointer to the underlying memory buffer that is given to the callers.
   */
  gsl::owner<std::byte *> arena_buffer;

  /**
   * @brief Determines if a number is a power of two, using bit operations.
   *
   * @param x The number to test
   * @return true If the number is a power of 2.
   * @return false It the numbe is not a power of 2.
   */
  [[nodiscard]] constexpr inline auto is_pow_2(std::size_t x) const -> bool {
    return (x & (x - 1)) == 0;
  }

  /**
   * @brief Modifies an allocation size to be aligned with the specified
   * alignment
   *
   * @param intended_size The size one wishes to allocate.
   * @param alignment The alignment of the allocation.
   * @return std::size_t The aligned allocation size.
   */
  [[nodiscard]] auto align_alloc_size(std::size_t intended_size,
                                      std::size_t alignment) -> std::size_t;
};

} // namespace surge

#endif // SURGE_LINEAR_ARENA_ALLOCATOR