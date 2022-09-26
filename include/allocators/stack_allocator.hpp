#ifndef SURGE_STACK_ALLOCATOR
#define SURGE_STACK_ALLOCATOR

#include "base_allocator.hpp"
#include "log.hpp"
#include "options.hpp"

#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <tuple>

namespace surge {

/**
 * @brief Saves the state of the allocator for use in rewinding
 *
 */
struct stack_allocator_state {
  std::size_t allocation_counter{0};
  std::size_t free_index{0};
};

/**
 * @brief A stack arena allocator.
 *
 * The stack arena allocator works by allocating a single contiguous stack of
 * memory and dispensing memory from the bottom of the stack each time an allocation is
 * performed. Objects are freed in a LIFO manner. If the block to be freed is not at the top of the
 * stack, the free request is ignored.
 *
 * The arena requires an internal allocator type, that is responsible for
 * allocating it's internal buffer.
 *
 * @tparam parent_allocator_t The parent allocator for the arena, responsible
 * for allocating it's internal memory block.
 */
class stack_allocator final : public base_allocator {
public:
  stack_allocator() noexcept
      : stack_buffer(nullptr, [&](void *ptr) { parent_allocator->free(ptr); }) {}

  ~stack_allocator() noexcept final = default;

  void init(base_allocator *pa, std::size_t capacity, const char *debug_name) noexcept;

  stack_allocator(const stack_allocator &) = delete;
  stack_allocator(stack_allocator &&) = delete;

  auto operator=(const stack_allocator &) -> stack_allocator & = delete;
  auto operator=(stack_allocator &&) -> stack_allocator & = delete;

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
   * @brief Reallocates the given area of memory if it is at the top of the stack, otherwise
   * allocates a new block of memory. If ptr is a null pointer, realloc() shall be equivalent to
   * malloc() for the specified size.
   *
   * @param ptr Pointer to the memore area to be reallocated.
   *
   * @param new_size New size of the memory block.
   *
   * @return On success, returns a pointer to the beginning of newly allocated
   * memory.
   *
   * @return On failure or if ptr is invalid, returns a null pointer.
   */
  [[nodiscard]] auto realloc(void *ptr, std::size_t new_size) noexcept -> void * final;

  /**
   * @brief Releases previuslly allocated memory if the block is at the top of the stack, otherwise
   * the request is ignored.
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
  [[nodiscard]] auto save() const noexcept -> stack_allocator_state;

  /**
   * @brief Restores the allocator to the saved point
   *
   */
  void restore(const stack_allocator_state &) noexcept;

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
  const char *allocator_debug_name{"Stack allocator"};
#endif

  /**
   * @brief Counts how many allocations have been made. Used to determine if a block is at the stack
   * top during a free operation.
   */
  std::size_t allocation_counter{0};

  /**
   * Index to the first free adress in the underlying memory buffer
   */
  std::size_t free_index{0};

  /**
   * @brief The size of the allocation header.
   */
  const std::size_t header_size{sizeof(std::size_t)};

  /**
   * Pointer to the underlying memory buffer that is given to the callers.
   */
  std::unique_ptr<std::byte, std::function<void(void *)>> stack_buffer;

  /**
   * @brief Detect if a pointer is valid. A pointer is considered valid if:
   * 1. It is not null
   * 2. Is inside the internal buffer
   * 3. Has a header <= the allocation counter.
   *
   * @param ptr The pointer to test
   * @return true If this allocator has allocated this adress.
   * @return false If the allocator has not allocated this address
   */
  auto is_valid(void *ptr) noexcept -> bool;

  /**
   * @brief Detect if a pointer points to the last block of allocated memory (stack top)
   *
   * @param ptr The pointer to test. Assumed to be valid according to is_valid
   * @return true If the pointer is the last allocated block (stack top)
   * @return false If the pointer is not the last allocated block (stack top)
   */
  auto is_last_block(void *ptr) noexcept -> bool;

  /**
   * @brief Converts a valid pointer to arena buffer index.
   *
   * @param ptr Pointer to the data block to convert to indices.
   * @return std::tuple<std::size_t, std::size_t> A tuple containing the header start and data start
   * indices .
   */
  auto ptr_to_idx(void *ptr) const noexcept -> std::tuple<std::size_t, std::size_t>;

  /**
   * @brief Reads the header of the block pointed by a valid pointer
   *
   * @param ptr Pointer to the data block to read.
   * @return std::size_t The header of the data block
   */
  auto read_header(void *ptr) const noexcept -> std::size_t;
};

} // namespace surge

#endif // SURGE_STACK_ALLOCATOR