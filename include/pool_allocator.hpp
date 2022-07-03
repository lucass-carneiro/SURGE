#ifndef SURGE_MEMORY_POOL_ALLOCATOR_HPP
#define SURGE_MEMORY_POOL_ALLOCATOR_HPP

#include "arena_allocator.hpp"
#include "options.hpp"

//clang-format off

#ifdef SURGE_DEBUG_MEMORY
#define EASTL_NAME_ENABLED 1 // NOLINT(cppcoreguidelines-macro-usage)
#endif

#include <EASTL/allocator.h>
#include <EASTL/vector.h>

//clang-format on

#include <cstddef>
#include <mutex>
#include <vector>

namespace surge {

/**
 * TODO: Finish this allocator/acess if it is really necessary
 * Pool allocator compatible with EASTL
 * https://eastl.docsforge.com/master/faq/#cont27-how-do-i-make-two-containers-share-a-memory-pool
 */
class pool_allocator {
public:
/**
 * Create an arena allocator.
 *
 * @param name The name of the allocator, for debugging purpouses.
 * @param np The number of pools to allocate.
 * @param ps The size of each pool.
 */
#ifdef SURGE_DEBUG_MEMORY
  pool_allocator(const char *name, std::size_t np, std::size_t ps) noexcept
      : debug_name{name}, num_pools{np}, pool_size{ps},
        buffer_size{num_pools * pool_size}, backing_buffer{buffer_size} {}
#else
#endif

  /**
   * Returns the allocator's debug name.
   *
   * @return The allocator's debug name.
   */
  [[nodiscard]] auto get_name() const -> const char * {
    std::lock_guard lock(pool_mutex);
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
    std::lock_guard lock(pool_mutex);
    debug_name = name;
  }
#else
  void set_name(const char *) {}
#endif

private:
#if EASTL_NAME_ENABLED
  /**
   * The debug name of the allocator
   */
  const char *debug_name;
#endif

  /**
   * The number of pool chunks
   */
  const std::size_t num_pools;

  /**
   * The size of each pool chunk
   */
  const std::size_t pool_size;

  /**
   * The size of the pool's backing buffer
   */
  const std::size_t buffer_size;

  /**
   * The underlying memory buffer that is given to the callers.
   */
  eastl::vector<std::byte> backing_buffer;

  /**
   * Protects multithreaded operations
   */
  mutable std::mutex pool_mutex;
};

} // namespace surge

#endif // SURGE_MEMORY_POOL_ALLOCATOR_HPP