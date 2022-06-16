#include "arena_allocator.hpp"

#include "log.hpp"

#include <cstddef>
#include <mutex>
#include <vulkan/vulkan_core.h>

#ifdef SURGE_DEBUG_MEMORY

surge::arena_allocator::~arena_allocator() {
  std::lock_guard lock(arena_mutex);

  if (allocs != 0) {
    global_stdout_log::instance().log<log_event::warning>(
        "Dangling pointer risk ahead! Not all allocations in {} were freed "
        "and complete arena deallocation is about to take place. Procead with cauton");
  }
}

void surge::arena_allocator::deallocate(void *p, std::size_t n) {
  std::lock_guard lock(arena_mutex);

  global_stdout_log::instance().log<log_event::message>(
      "{} deallocated {} bytes starting at RAM address {:#x}", debug_name, n,
      reinterpret_cast<uintptr_t>(p) // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
  );
  allocs--;
}

#else

void surge::arena_allocator::deallocate(void *p, std::size_t n) {
  std::lock_guard lock(arena_mutex);
  allocs--;
}

#endif

auto surge::arena_allocator::allocate(std::size_t n, int flags) -> void * {
  return allocate(n, sizeof(void *), 0, flags);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto surge::arena_allocator::allocate(std::size_t n, std::size_t alignment, std::size_t offset,
                                      int flags) -> void * {

  std::lock_guard lock(arena_mutex);

  // TODO: what to do with the flags?
  (void)flags;

  if (alignment == 0) {
#ifdef SURGE_DEBUG_MEMORY
    global_stdout_log::instance().log<log_event::error>(
        "Unable to allocate {} bytes in {} with zero alignment.", n, debug_name);
#endif
    return nullptr;
  }

  // Push the index to be a multiple of the alignment
  std::size_t modulo{0};

  if (!is_pow_2(alignment)) {
#ifdef SURGE_DEBUG_MEMORY
    global_stdout_log::instance().log<log_event::warning>(
        "{} bytes allocation in {} requested an alignment of {}, which is "
        "not a power of 2.",
        n, debug_name, alignment);
#endif
    modulo = free_index % alignment;
  } else {
    modulo = free_index & (alignment - 1);
  }

  // TODO: Is it possible that (alignment - modulo) < 0? If so, this must be prevented.
  const std::size_t intended_alloc_idx = free_index + (alignment - modulo) + offset;

  if (intended_alloc_idx > arena_capacity) {
#ifdef SURGE_DEBUG_MEMORY
    global_stdout_log::instance().log<log_event::error>(
        "Unable to allocate {} bytes in {} since tis would result in an "
        "actual allocation of {} bytes and there are only {} bytes left.",
        n, debug_name, intended_alloc_idx - free_index, arena_capacity - free_index);
#endif
    return nullptr;
  }

  const auto start_index = free_index;
  free_index = intended_alloc_idx;
  allocs++;
  auto start_ptr = &(data_buffer[start_index]);

#ifdef SURGE_DEBUG_MEMORY
  global_stdout_log::instance().log<log_event::message>(
      // log_all<log_event::message>(
      "{} allocation summary:\n"
      "  Requested size {}\n"
      "  Allocated size {}\n"
      "  Internal range ({},{})\n"
      "  RAM address {:#x}",
      debug_name, n, free_index - start_index, start_index, free_index - 1,
      reinterpret_cast<uintptr_t>(start_ptr) // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
  );
#endif
  return static_cast<void *>(start_ptr);
}

void surge::arena_allocator::reset() {
  std::lock_guard lock(arena_mutex);

  free_index = 0;
  allocs = 0;
}