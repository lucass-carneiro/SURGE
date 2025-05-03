#include "sc_allocators.hpp"

#include "sc_logging.hpp"

#include <mimalloc.h>

#ifdef SURGE_ENABLE_TRACY
#  include <tracy/Tracy.hpp>
#endif

namespace surge::allocators {

auto mimalloc::malloc(usize size) -> void * {
  auto p{mi_malloc(size)};
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Memory Event\n"
            "---\n"
            "type: alloc\n"
            "allocator: \"mimalloc::malloc\"\n"
            "size: {}\n"
            "address: {}\n"
            "failed: {}",
            size, p, p ? "false" : "true");
#endif
  return p;
}

void mimalloc::free(void *p) {
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Memory Event\n"
            "---\n"
            "type: free\n"
            "allocator: \"mimalloc::free\"\n"
            "address: {}",
            p);
#endif
  mi_free(p);
}

auto surge::allocators::mimalloc::aligned_alloc(usize size, usize alignment) -> void * {
  auto p{mi_aligned_alloc(alignment, size)};
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Memory Event\n"
            "---\n"
            "type: alloc\n"
            "allocator: \"mimalloc::mi_aligned_alloc\"\n"
            "size: {}\n"
            "alignment: {}\n"
            "address: {}\n"
            "failed: {}",
            size, alignment, p, p ? "false" : "true");
#endif
  return p;
}

auto surge::allocators::mimalloc::aligned_realloc(void *p, usize newsize,
                                                  usize alignment) -> void * {
  auto q{mi_realloc_aligned(p, newsize, alignment)};
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Memory Event\n"
            "---\n"
            "type: realloc\n"
            "allocator: \"mimalloc::mi_realloc_aligned\"\n"
            "new size: {}\n"
            "alignent: {}\n",
            "old address: {}\n"
            "new address: {}\n"
            "failed: {}",
            newsize, alignment, p, q, q ? "false" : "true");
#endif
  return q;
}

void mimalloc::init() {
  // see https://microsoft.github.io/mimalloc/group__options.html
#ifdef SURGE_DEBUG_MEMORY
  mi_option_enable(mi_option_show_errors);
  mi_option_enable(mi_option_show_stats);
  mi_option_enable(mi_option_verbose);
#else
  mi_option_disable(mi_option_show_errors);
  mi_option_disable(mi_option_show_stats);
  mi_option_disable(mi_option_verbose);
#endif

  mi_option_enable(mi_option_eager_commit);
  mi_option_enable(mi_option_disallow_os_alloc);

  mi_option_set(mi_option_reserve_huge_os_pages, 1);
  mi_option_set(mi_option_eager_commit_delay, 100);
}

auto DynamicArena::is_pow_2(surge::usize x) const noexcept -> bool { return (x & (x - 1)) == 0; }

DynamicArena::DynamicArena(usize initial_size, const char *name) noexcept : arena_name(name) {
  using std::memset;

  log_info("Allocating initial {} B for arena \"{}\"", initial_size, arena_name);

  memory_data = static_cast<std::byte *>(mimalloc::malloc(initial_size));

  if (memory_data == nullptr) {
    log_error("Unable to initially allocate {} B for arena \"{}\"", initial_size, arena_name);
    return;
  } else {
    memset(memory_data, 0, initial_size);
    current_capacity = initial_size;

    log_info("Arena \"{}\" ready to operate with initial {} B of capacity", arena_name,
             initial_size);
  }
}

DynamicArena::~DynamicArena() noexcept {
  log_info("Deallocating arena \"{}\" with current size {}, current capacity {}", arena_name,
           current_offset, current_capacity);
  mimalloc::free(memory_data);
}

auto DynamicArena::allocate(std::size_t size, std::size_t alignment) -> void * {
  using std::memset, std::memcpy;
  std::lock_guard<std::mutex> lock{mutex};

  if (!is_pow_2(alignment)) {
    log_error("{} B allocation on arena \"{}\" failed: Alignment {} is not a power of 2", size,
              arena_name, alignment);
    throw std::bad_alloc();
  }

  const auto modulo{size & (alignment - 1)}; // same as size % alignment when a is a power of 2
  const auto actual_size{size + modulo};
  const auto new_offset{current_offset + actual_size};

  if (new_offset >= current_capacity) {
    const auto new_capacity{current_capacity * 2};

    log_info("Growing arena \"{}\" from {} B to {} B of capacity", arena_name, current_capacity,
             new_capacity);

    auto new_memory_data{static_cast<std::byte *>(mimalloc::malloc(new_capacity))};

    if (new_memory_data == nullptr) {
      log_error("Failed to grow arena \"{}\"", arena_name);
      throw std::bad_alloc();
    }

    memcpy(new_memory_data, memory_data, current_offset);
    mimalloc::free(memory_data);

    current_capacity = new_capacity;
    memory_data = new_memory_data;
  }

  auto p{static_cast<void *>(&(memory_data[current_offset]))};
  current_offset = new_offset;
  memset(p, 0, actual_size);

#ifdef SURGE_DEBUG_MEMORY
  log_debug("Memory Event\n"
            "---\n"
            "type: alloc\n"
            "allocator: \"{}\"\n "
            "size: {}\n"
            "alignment: {}\n"
            "total aligned size: {}\n"
            "address: {}\n",
            arena_name, size, alignment, actual_size, p);
#endif

  return p;
}

void DynamicArena::reset() noexcept {
  std::lock_guard<std::mutex> lock{mutex};
  current_offset = 0;
}

} // namespace surge::allocators