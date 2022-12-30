#include "allocators/default_allocator.hpp"

#include "log.hpp"

// clang-format off
#include <mimalloc.h>
// clang-format on

#include <cerrno>
#include <cstring>

[[nodiscard]] auto surge::default_allocator::malloc(std::size_t size) noexcept -> void * {
  // NOLINTNEXTLINE
  void *buffer{mi_malloc(size)};

#ifdef SURGE_DEBUG_MEMORY
  if (buffer != nullptr) {
    glog<log_event::memory>("Allocator \"{}\" (malloc) allocation summary:\n"
                            "  Allocated size {}\n"
                            "  RAM address {:#x}",
                            allocator_debug_name, size,
                            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                            reinterpret_cast<std::uintptr_t>(buffer));
  } else {
    glog<log_event::error>("In allocator \"{}\"(malloc): Allocation of size {} failed. Reason: {}",
                           allocator_debug_name, size, std::strerror(errno));
  }
#endif

  return buffer;
}

[[nodiscard]] auto surge::default_allocator::aligned_alloc(std::size_t alignment,
                                                           std::size_t size) noexcept -> void * {
  // NOLINTNEXTLINE
  void *buffer{mi_aligned_alloc(alignment, size)};

#ifdef SURGE_DEBUG_MEMORY
  if (buffer != nullptr) {
    glog<log_event::memory>("Allocator \"{}\" (alligned_alloc) allocation summary:\n"
                            "  Allocated size {}\n"
                            "  RAM address {:#x}",
                            allocator_debug_name, size,
                            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                            reinterpret_cast<std::uintptr_t>(buffer));
  } else {
    glog<log_event::error>(
        "In allocator \"{}\"(aligned_alloc): Allocation of size {} failed. Reason: {}",
        allocator_debug_name, size, std::strerror(errno));
  }
#endif

  return buffer;
}

[[nodiscard]] auto surge::default_allocator::calloc(std::size_t num, std::size_t size) noexcept
    -> void * {

  // NOLINTNEXTLINE
  void *buffer{mi_calloc(num, size)};

#ifdef SURGE_DEBUG_MEMORY
  if (buffer != nullptr) {
    glog<log_event::memory>("Allocator \"{}\" (calloc) allocation summary:\n"
                            "  Allocated size {}\n"
                            "  RAM address {:#x}",
                            allocator_debug_name, size,
                            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                            reinterpret_cast<std::uintptr_t>(buffer));
  } else {
    glog<log_event::error>("In allocator \"{}\"(calloc): Allocation of size {} failed. Reason: {}",
                           allocator_debug_name, size, std::strerror(errno));
  }
#endif

  return buffer;
}

[[nodiscard]] auto surge::default_allocator::realloc(void *ptr, std::size_t new_size) noexcept
    -> void * {

  // NOLINTNEXTLINE
  void *buffer{mi_realloc(ptr, new_size)};

#ifdef SURGE_DEBUG_MEMORY
  if (buffer != nullptr) {
    glog<log_event::memory>("Allocator \"{}\" reallocation summary:\n"
                            "  New allocated size {}\n"
                            "  RAM address {:#x}",
                            allocator_debug_name, new_size,
                            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                            reinterpret_cast<std::uintptr_t>(buffer));
  } else {
    glog<log_event::error>(
        "In allocator \"{}\"(realloc): Reallocation to new size {} failed. Reason: {}",
        allocator_debug_name, new_size, std::strerror(errno));
  }
#endif

  return buffer;
}

void surge::default_allocator::free(void *ptr) noexcept {
#ifdef SURGE_DEBUG_MEMORY
  glog<log_event::message>("Allocator \"{}\" released address {:#x}", allocator_debug_name,
                           // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                           reinterpret_cast<std::uintptr_t>(ptr));
#endif

  // NOLINTNEXTLINE
  mi_free(ptr);
}

#ifdef SURGE_DEBUG_MEMORY
void surge::default_allocator::init(const char *debug_name) noexcept {
  allocator_debug_name = debug_name;

  mi_option_set(mi_option_show_stats, 1);
  mi_option_set(mi_option_verbose, 1);
  mi_option_set(mi_option_show_errors, 1);
}
#else
void surge::default_allocator::init(const char *) noexcept {
  mi_option_set(mi_option_show_errors, 1);
}
#endif