#ifndef SURGE_ARENA_ALLOCATOR_HPP
#define SURGE_ARENA_ALLOCATOR_HPP

#include "options.hpp"

//clang-format off

#ifdef SURGE_DEBUG_MEMORY
#define EASTL_NAME_ENABLED 1 // NOLINT(cppcoreguidelines-macro-usage)
#endif

#include <EASTL/allocator.h>

//clang-format on

#include <cstddef>
#include <mutex>
#include <vector>

namespace surge {

/**
 * Arena allocator compatible with EASTL
 */
class arena_allocator {
public:
/**
 * Create an arena allocator.
 *
 * @param name The name of the allocator, for debugging purpouses.
 * @param capacity The total size of the arena.
 */
#ifdef SURGE_DEBUG_MEMORY
  arena_allocator(const char *name, std::size_t capacity) noexcept
      : debug_name{name}, arena_capacity{capacity}, free_index{0}, allocs{0},
        data_buffer{capacity} {}
#else
  arena_allocator(const char *name, std::size_t capacity) noexcept
      : arena_capacity{capacity}, free_index{0}, allocs{0}, data_buffer{
                                                                capacity} {}
#endif

  /**
   * Create a new arena by copying another.
   *
   * This constructor locks the mutex of the source object before copying
   *
   * @param other Const reference to an arena to copy.
   *
   */
  arena_allocator(const arena_allocator &other) noexcept
      : arena_allocator(other, std::lock_guard<std::mutex>(other.arena_mutex)) {
  }

  // Move constructor and assignment are deleted because they are not necessary
  arena_allocator(arena_allocator &&) noexcept = delete;
  auto operator=(arena_allocator &&) noexcept -> arena_allocator & = delete;

  /**
   * Swaps two arena objects data members.
   *
   * This function does not lock any of the object's mutexes since both mutexes
   * will be locked befor it is called.
   */
  friend inline void swap(arena_allocator &first, arena_allocator &second) {
    using std::swap;
#ifdef SURGE_DEBUG_MEMORY
    swap(first.debug_name, second.debug_name);
#endif
    swap(first.arena_capacity, second.arena_capacity);
    swap(first.free_index, second.free_index);
    swap(first.allocs, second.allocs);
    swap(first.data_buffer, second.data_buffer);
  }

  /**
   * Assign to an arena by copying other.
   *
   * This function locks the mutexes of both arenas.
   *
   * @param other Arena to assign from.
   */
  auto operator=(arena_allocator other) -> arena_allocator & {
    std::scoped_lock lock(arena_mutex, other.arena_mutex);
    swap(*this, other);
    return *this;
  };

#ifdef SURGE_DEBUG_MEMORY
  /**
   * Destroy the arena and free it's buffer memory
   *
   * Because SURGE_DEBUG_MEMORY is enabled, this destructor checks if all
   * allocate calls were paired with their respective deallocate calls. If there
   * is an alloc/free inbalence, the destructor warns the user of possible
   * dangling pointe errors.
   */
  ~arena_allocator();
#else
  /**
   * Destroy the arena and free it's buffer memory
   */
  ~arena_allocator() = default;
#endif

  /**
   * Allocates memory from the arena.
   *
   * This overload allocates memory with sizeof(void*) bytes of alignment and 0
   * offset. Increases the allocation counter by 1.
   *
   * @param n The ammount of memory (in bytes) to allocate.
   * @param flags User defined allocator flags. Currently unused and defaults to
   * 0.
   * @return Pointer to allocated memory address or nullptr in case of failure;
   */
  auto allocate(std::size_t n, int flags = 0) -> void *;

  /**
   * Allocates memory from the arena.
   *
   * Increases the allocation counter by 1.
   *
   * @param n The ammount of memory (in bytes) to allocate.
   * @param alignment The alignment to use for the newly allocated memory.
   * Powers of 2 prefered.
   * @param offeset The ofset to add to the aligned address.
   * @param flags User defined allocator flags. Currently unused and defaults to
   * 0.
   * @return Pointer to allocated memory address or nullptr in case of failure;
   */
  auto allocate(std::size_t n, std::size_t alignment, std::size_t offset,
                int flags = 0) -> void *;

  /**
   * Free allocated memory.
   *
   * Simply decreases the memory counter by 1 but does not drop the arena (this
   * is only done when the destructor is called)
   *
   * @param p Pointer to the begining of the memory block to free.
   * @param n Size of the memory block to be freed.
   */
  void deallocate(void *p, std::size_t n);

  /**
   * Returns the allocator's debug name.
   *
   * @return The allocator's debug name.
   */
  [[nodiscard]] auto get_name() const -> const char * {
    std::lock_guard lock(arena_mutex);
#if EASTL_NAME_ENABLED
    return debug_name;
#else
    return "unnamed arena allocator";
#endif
  }

  /**
   * Sets/changes the current allocator name.
   *
   * @param name The allocator name to use.
   */
#if EASTL_NAME_ENABLED
  void set_name(const char *name) {
    std::lock_guard lock(arena_mutex);
    debug_name = name;
  }
#else
  void set_name(const char *) {}
#endif

  /**
   * Resets the arena to it's initial configuration. This allows it to be reused
   * for new allocations. This also means that any previously existing memory
   * pointers are invalidated after this call.
   */
  void reset();

private:
#if EASTL_NAME_ENABLED
  /**
   * The debug name of the allocator
   */
  const char *debug_name;
#endif

  /**
   * The total memory the arena can provide.
   */
  std::size_t arena_capacity;

  /**
   * Index to the first free adress in the underlying memory buffer
   */
  std::size_t free_index;

  /**
   * Number of allocate calls performed.
   */
  std::size_t allocs;

  /**
   * The underlying memory buffer that is given to the callers.
   */
  std::vector<std::byte> data_buffer;

  /**
   * Protects multithreaded operations
   */
  mutable std::mutex arena_mutex;

  /**
   * Determines if a number is a power of 2
   *
   * @param x The number to be checked.
   * @return true if the number is a power of 2, false otherwise.
   */
  [[nodiscard]] constexpr inline auto is_pow_2(std::size_t x) const -> bool {
    return (x & (x - 1)) == 0;
  }

  /**
   * Copy initialize an arena in a thread safe maner by locking the source mutex
   *
   * @param other The source arena.
   */
  arena_allocator(const arena_allocator &other,
                  const std::lock_guard<std::mutex> &)
      : debug_name{other.debug_name}, arena_capacity{other.arena_capacity},
        free_index{other.free_index}, allocs{other.allocs},
        data_buffer{other.data_buffer} {}
};

/**
 * Tests if two arena allocators are the same.
 *
 * Two arenas are the same if TODO: Implement
 *
 * @param lhs The left hand side of the equality operator.
 * @param rhs The right hand side of the equality operator.
 */
inline auto operator==(const eastl::allocator &, const eastl::allocator &)
    -> bool {
  return false;
}

/**
 * Tests if two arena allocators are different.
 *
 * Two arenas different if TODO: Implement
 *
 * @param lhs The left hand side of the equality operator.
 * @param rhs The right hand side of the equality operator.
 */
inline auto operator!=(const eastl::allocator &, const eastl::allocator &)
    -> bool {
  return true;
}

class global_arena_allocator {
public:
  static auto get() noexcept -> arena_allocator & {
    static arena_allocator allocator("Global arena allocator", arena_size);
    return allocator;
  }

  static const std::size_t arena_size;

  global_arena_allocator(const global_arena_allocator &) = delete;
  global_arena_allocator(global_arena_allocator &&) = delete;

  auto operator=(global_arena_allocator) -> global_arena_allocator & = delete;

  auto operator=(const global_arena_allocator &)
      -> global_arena_allocator & = delete;

  auto operator=(global_arena_allocator &&)
      -> global_arena_allocator & = delete;

  ~global_arena_allocator() = default;

private:
  global_arena_allocator() = default;
};

} // namespace surge

/**
 * Global memory allocator used by EASTL. Wrapps the global_arena_allocator.
 *
 * @param size The number of bytes to allocate.
 * @param pName The allocator name. (TODO: Is this correct?)
 * @param flags Allocator control flags. (TODO: Is this correct?)
 * @param debugFlags Allocator debug flags. (TODO: Is this correct?)
 * @param file Name of the file calling the new(). (TODO: Is this correct?)
 * @param line Number of the line calling new(). (TODO: Is this correct?)
 * @return Pointer to new memory or null.
 */
auto operator new[](size_t size, const char *pName, int flags,
                    unsigned debugFlags, const char *file, int line) -> void *;

/**
 * Global memory allocator used by EASTL. Wrapps the global_arena_allocator.
 *
 * @param size The number of bytes to allocate.
 * @param alignment Allocation alignment.
 * @param alignmentOffset Allocation offset.
 * @param pName The allocator name. (TODO: Is this correct?)
 * @param flags Allocator control flags. (TODO: Is this correct?)
 * @param debugFlags Allocator debug flags. (TODO: Is this correct?)
 * @param file Name of the file calling the new(). (TODO: Is this correct?)
 * @param line Number of the line calling new(). (TODO: Is this correct?)
 * @return Pointer to new memory or null.
 */
auto operator new[](size_t size, size_t alignment, size_t alignmentOffset,
                    const char *pName, int flags, unsigned debugFlags,
                    const char *file, int line) -> void *;

#endif // SURGE_ARENA_ALLOCATOR_HPP