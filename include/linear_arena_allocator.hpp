#ifndef SURGE_LINEAR_ARENA_ALLOCATOR
#define SURGE_LINEAR_ARENA_ALLOCATOR

#include "allocators.hpp"
#include "log.hpp"
#include "options.hpp"

#include <gsl/gsl-lite.hpp>

#include <cstddef>
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
template <surge_allocator parent_allocator_t = default_allocator>
class linear_arena_allocator {
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
  linear_arena_allocator(parent_allocator_t &pa, std::size_t capacity,
                         const char *debug_name)
      : parent_allocator{pa}, requested_arena_capacity{capacity},
        actual_arena_capacity{
            align_alloc_size(requested_arena_capacity, default_alignment)},
        allocator_debug_name{debug_name},
        arena_buffer{static_cast<std::byte *>(
            pa.aligned_alloc(default_alignment, actual_arena_capacity))} {}

  /**
   * @brief Destroy the linear arena allocator object.
   *
   */
  ~linear_arena_allocator() {
    parent_allocator.free(arena_buffer);

#ifdef SURGE_DEBUG_MEMORY
    log_all<log_event::message>(
        "Allocator \"{}\"  was destroyed with {} remaining allocations.",
        allocator_debug_name, allocation_counter);
#endif
  }

#else
// TODO
#endif

  linear_arena_allocator(const linear_arena_allocator &) = delete;
  linear_arena_allocator(linear_arena_allocator &&) = delete;

  auto operator=(linear_arena_allocator) -> linear_arena_allocator & = delete;
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
                                   std::size_t size) noexcept -> void * {

#ifdef SURGE_DEBUG_MEMORY
    // Precondition 1: alignment must be a power of 2
    if (!is_pow_2(alignment)) {
      log_all<log_event::error>(
          "In allocator \"{}\"(aligned_alloc): Allocation of alignment {} "
          "requested, which is not a power of 2",
          allocator_debug_name, alignment);

      return nullptr;
    }

    // Precondition 2: alignment is a multiple of sizeof(void*)
    if ((alignment % sizeof(void *)) != 0) {
      log_all<log_event::error>(
          "In allocator \"{}\"(aligned_alloc): Allocation of alignment {} "
          "requested, which is not a multiple of sizeof(void*), which is {}",
          allocator_debug_name, alignment, sizeof(void *));

      return nullptr;
    }

    // Precondition 3: size is a non zero integral multiple of alignment
    if (size == 0 || (size % alignment != 0)) {
      log_all<log_event::error>(
          "In allocator \"{}\"(aligned_alloc): Allocation of size {} "
          "requested, "
          "which is not a non zero integral muliple of the alignment {}",
          allocator_debug_name, size, alignment);

      return nullptr;
    }
#endif

    const auto current_index = free_index;
    const auto bumped_index = current_index + size;

    if (bumped_index >= actual_arena_capacity) {
#ifdef SURGE_DEBUG_MEMORY
      log_all<log_event::error>(
          "In allocator \"{}\"(aligned_alloc): Allocation of size {} requested "
          "exceeds the arena capacity of (requested, actual) ({}, {})",
          allocator_debug_name, size, requested_arena_capacity,
          actual_arena_capacity);
#endif
      return nullptr;
    }

    // Allocation succesfull
    free_index = bumped_index;
    std::byte *start_ptr = &(arena_buffer[current_index]);

#ifdef SURGE_DEBUG_MEMORY
    log_all<log_event::message>(
        "Allocator \"{}\" allocation summary:\n"
        "  Allocated size {}\n"
        "  Internal range ({},{})\n"
        "  RAM address {:#x}",
        allocator_debug_name, size, current_index, free_index - 1,
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<std::uintptr_t>(start_ptr));

    allocation_counter++;
#endif

    return static_cast<void *>(start_ptr);
  }

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
  [[nodiscard]] inline auto malloc(std::size_t size) noexcept -> void * {

#ifdef SURGE_DEBUG_MEMORY
    // Precondition 1: size must be non null
    if (size == 0) {
      log_all<log_event::error>(
          "In allocator \"{}\"(malloc): Allocation of size 0 is ill defined",
          allocator_debug_name);

      return nullptr;
    }
#endif

    const auto actual_alloc_size = align_alloc_size(size, default_alignment);

    return this->aligned_alloc(default_alignment, actual_alloc_size);
  }

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
  [[nodiscard]] inline auto calloc(std::size_t num,
                                   std::size_t size) const noexcept -> void * {
#ifdef SURGE_DEBUG_MEMORY
    // Precondition 1: num must be non null
    if (num == 0) {
      log_all<log_event::error>(
          "In allocator \"{}\"(calloc): Allocation of 0 elements ill defined",
          allocator_debug_name);

      return nullptr;
    }

    // Precondition 2: size must be non null
    if (size == 0) {
      log_all<log_event::error>(
          "In allocator \"{}\"(calloc): Allocation of size "
          "0 elements ill defined",
          allocator_debug_name);

      return nullptr;
    }
#endif

    const auto intended_alloc_size{num * size};
    const auto actual_alloc_size =
        align_alloc_size(intended_alloc_size, default_alignment);

    auto memory = this->aligned_alloc(default_alignment, actual_alloc_size);

    if (memory != nullptr) {
      std::memset(memory, 0, actual_alloc_size);
    }

    return memory;
  }

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
  [[nodiscard]] auto realloc(void *ptr, std::size_t new_size) const noexcept
      -> void * {
    this->free(ptr);
    return malloc(new_size);
  }

  /**
   * @brief Releases previoslly allocated memory. Since the arena allocator
   * cannot free individual objects, this function does if the flag
   * SURGE_DEBUG_MEMORY is not set. If it is, it decreases the allocation
   * counter.
   *
   * @param ptr Pointer to the memory to deallocate.
   */
  void free(void *ptr) noexcept {
#ifdef SURGE_DEBUG_MEMORY
    log_all<log_event::message>(
        "Allocator \"{}\" released address {:#x}", allocator_debug_name,
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<std::uintptr_t>(ptr));

    allocation_counter--;
#endif
  }

  /**
   * @brief Resets the arena to it's initial configuration. This allows it to
   * be reused for new allocations. This also means that any previously
   * existing pointers are invalidated after this call.
   */
  void reset() {
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
  parent_allocator_t &parent_allocator;

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

  [[nodiscard]] constexpr inline auto
  align_alloc_size(std::size_t intended_size, std::size_t alignment)
      -> std::size_t {

    std::size_t modulo{0};

    if (is_pow_2(alignment)) {
      modulo = intended_size & (alignment - 1);
    } else {
      modulo = intended_size % alignment;
    }

    return intended_size + (alignment - modulo);
  }
};

} // namespace surge

#endif // SURGE_LINEAR_ARENA_ALLOCATOR