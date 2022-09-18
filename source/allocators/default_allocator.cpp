// TODO: use https://github.com/mjansson/rpmalloc ?

#include "allocators/default_allocator.hpp"

#include "log.hpp"

#include <cerrno>
#include <cstdlib>
#include <cstring>

[[nodiscard]] auto surge::default_allocator::malloc(std::size_t size) noexcept -> void * {
  // NOLINTNEXTLINE
  void *buffer{std::malloc(size)};

#ifdef SURGE_DEBUG_MEMORY
  if (buffer != nullptr) {
    log_all<log_event::memory>("Allocator \"{}\" (malloc) allocation summary:\n"
                               "  Allocated size {}\n"
                               "  RAM address {:#x}",
                               allocator_debug_name, size,
                               // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                               reinterpret_cast<std::uintptr_t>(buffer));
  } else {
    log_all<log_event::error>(
        "In allocator \"{}\"(malloc): Allocation of size {} failed. Reason: {}",
        allocator_debug_name, size, std::strerror(errno));
  }
#endif

  return buffer;
}

[[nodiscard]] auto surge::default_allocator::aligned_alloc(std::size_t alignment,
                                                           std::size_t size) noexcept -> void * {
  // NOLINTNEXTLINE
  void *buffer{std::aligned_alloc(alignment, size)};

#ifdef SURGE_DEBUG_MEMORY
  if (buffer != nullptr) {
    log_all<log_event::memory>("Allocator \"{}\" (alligned_alloc) allocation summary:\n"
                               "  Allocated size {}\n"
                               "  RAM address {:#x}",
                               allocator_debug_name, size,
                               // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                               reinterpret_cast<std::uintptr_t>(buffer));
  } else {
    log_all<log_event::error>(
        "In allocator \"{}\"(aligned_alloc): Allocation of size {} failed. Reason: {}",
        allocator_debug_name, size, std::strerror(errno));
  }
#endif

  return buffer;
}

[[nodiscard]] auto surge::default_allocator::calloc(std::size_t num, std::size_t size) noexcept
    -> void * {

  // NOLINTNEXTLINE
  void *buffer{std::calloc(num, size)};

#ifdef SURGE_DEBUG_MEMORY
  if (buffer != nullptr) {
    log_all<log_event::memory>("Allocator \"{}\" (calloc) allocation summary:\n"
                               "  Allocated size {}\n"
                               "  RAM address {:#x}",
                               allocator_debug_name, size,
                               // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                               reinterpret_cast<std::uintptr_t>(buffer));
  } else {
    log_all<log_event::error>(
        "In allocator \"{}\"(calloc): Allocation of size {} failed. Reason: {}",
        allocator_debug_name, size, std::strerror(errno));
  }
#endif

  return buffer;
}

[[nodiscard]] auto surge::default_allocator::realloc(void *ptr, std::size_t new_size) noexcept
    -> void * {

  // NOLINTNEXTLINE
  void *buffer{std::realloc(ptr, new_size)};

#ifdef SURGE_DEBUG_MEMORY
  if (buffer != nullptr) {
    log_all<log_event::memory>("Allocator \"{}\" reallocation summary:\n"
                               "  New allocated size {}\n"
                               "  RAM address {:#x}",
                               allocator_debug_name, new_size,
                               // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                               reinterpret_cast<std::uintptr_t>(buffer));
  } else {
    log_all<log_event::error>(
        "In allocator \"{}\"(realloc): Reallocation to new size {} failed. Reason: {}",
        allocator_debug_name, new_size, std::strerror(errno));
  }
#endif

  return buffer;
}

void surge::default_allocator::free(void *ptr) noexcept {
#ifdef SURGE_DEBUG_MEMORY
  log_all<log_event::memory>("Allocator \"{}\" released address {:#x}", allocator_debug_name,
                             // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                             reinterpret_cast<std::uintptr_t>(ptr));
#endif

  // NOLINTNEXTLINE
  std::free(ptr);
}

#ifdef SURGE_DEBUG_MEMORY
surge::default_allocator::~default_allocator() noexcept {
  log_all<log_event::memory>("Allocator \"{}\"  was destroyed with.", allocator_debug_name);
}
#endif