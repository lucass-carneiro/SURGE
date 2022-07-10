#include "arena_allocator.hpp"

#include "log.hpp"

#include <cstddef>
#include <iostream>
#include <mutex>
#include <vulkan/vulkan_core.h>

#ifdef SURGE_DEBUG_MEMORY

surge::arena_allocator::~arena_allocator() {
  std::lock_guard lock(arena_mutex);

  if (allocs != 0) {
    // TODO: This should be output by the logger, but the datetime string causes
    // a segfault
    std::cout << "Dangling pointer risk ahead! Not all allocations in \""
              << debug_name
              << "\" were freed "
                 "and complete arena deallocation is about to take place. "
                 "Procead with "
                 "caution"
              << std::endl;
  }
}

void surge::arena_allocator::deallocate(void *p, std::size_t n) {
  std::lock_guard lock(arena_mutex);

  log_all<log_event::message>(
      "{} deallocated {} bytes starting at RAM address {:#x}", debug_name, n,
      reinterpret_cast<uintptr_t>(
          p) // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
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
auto surge::arena_allocator::allocate(std::size_t n, std::size_t alignment,
                                      std::size_t offset, int flags) -> void * {

  std::lock_guard lock(arena_mutex);

  // TODO: what to do with the flags?
  (void)flags;

  if (alignment == 0) {
#ifdef SURGE_DEBUG_MEMORY
    log_all<log_event::error>(
        "Unable to allocate {} bytes in {} with zero alignment.", n,
        debug_name);
#endif
    return nullptr;
  }

  // Push the index to be a multiple of the alignment
  std::size_t modulo{0};

  if (!is_pow_2(alignment)) {
#ifdef SURGE_DEBUG_MEMORY
    log_all<log_event::warning>(
        "{} bytes allocation in {} requested an alignment of {}, which is "
        "not a power of 2.",
        n, debug_name, alignment);
#endif
    modulo = free_index % alignment;
  } else {
    modulo = free_index & (alignment - 1);
  }

  // TODO: Is it possible that (alignment - modulo) < 0? If so, this must be
  // prevented.
  const std::size_t end_index = free_index + n + (alignment - modulo) + offset;
  const std::size_t actual_alloc_size = n + (alignment - modulo) + offset;
  const std::size_t bytes_left = arena_capacity - free_index;

  if (end_index > arena_capacity) {
#ifdef SURGE_DEBUG_MEMORY
    log_all<log_event::error>(
        "Unable to allocate {} bytes in {} since tis would result in an "
        "actual allocation of {} bytes and there are only {} bytes left.",
        n, debug_name, actual_alloc_size, bytes_left);
#endif
    return nullptr;
  }

  const auto start_index = free_index;
  free_index += actual_alloc_size;
  allocs++;
  auto start_ptr = &(data_buffer[start_index]);

#ifdef SURGE_DEBUG_MEMORY
  log_all<log_event::message>(
      "{} allocation summary:\n"
      "  Requested size {}\n"
      "  Allocated size {}\n"
      "  Internal range ({},{})\n"
      "  RAM address {:#x}",
      debug_name, n, free_index - start_index, start_index, free_index - 1,
      reinterpret_cast<uintptr_t>(
          start_ptr) // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
  );
#endif
  return static_cast<void *>(start_ptr);
}

void surge::arena_allocator::reset() {
  std::lock_guard lock(arena_mutex);

  free_index = 0;
  allocs = 0;
  data_buffer.clear();
}

auto operator new[](size_t size, const char *pName, int flags,
                    unsigned debugFlags, const char *file, int line) -> void * {
  // TODO: What to do with those flags?
  (void)pName;
  (void)debugFlags;
  (void)file;
  (void)line;
  return EASTLAllocFlags("dummy", size, flags);
}

auto operator new[](size_t size, size_t alignment, size_t alignmentOffset,
                    const char *pName, int flags, unsigned debugFlags,
                    const char *file, int line) -> void * {

  // TODO: What to do with those flags?
  (void)pName;
  (void)debugFlags;
  (void)file;
  (void)line;
  return EASTLAllocAlignedFlags("dummy", size, alignment, alignmentOffset,
                                flags);
}