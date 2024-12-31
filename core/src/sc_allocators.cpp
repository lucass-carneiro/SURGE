#include "sc_allocators.hpp"

#include "sc_logging.hpp"
#include "sc_options.hpp"

#include <mimalloc.h>
#include <stdexcept>

#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#endif

auto surge::allocators::mimalloc::malloc(usize size) -> void * {
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

auto surge::allocators::mimalloc::realloc(void *p, usize newsize) -> void * {
  auto q{mi_realloc(p, newsize)};
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Memory Event\n"
            "---\n"
            "type: realloc\n"
            "allocator: \"mimalloc::realloc\"\n"
            "new size: {}\n"
            "old address: {}\n"
            "new address: {}\n"
            "failed: {}",
            newsize, p, q, q ? "false" : "true");
#endif
  return q;
}

auto surge::allocators::mimalloc::calloc(usize count, usize size) -> void * {
  auto p{mi_calloc(count, size)};
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Memory Event\n"
            "---\n"
            "type: alloc\n"
            "allocator: \"mimalloc::calloc\"\n"
            "size: {}\n"
            "count: {}\n"
            "address: {}\n"
            "failed: {}",
            size, count, p, p ? "false" : "true");
#endif
  return p;
}

void surge::allocators::mimalloc::free(void *p) {
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

auto surge::allocators::mimalloc::aligned_realloc(void *p, usize newsize, usize alignment)
    -> void * {
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

void surge::allocators::mimalloc::aligned_free(void *p, usize alignment) {
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Memory Event\n"
            "---\n"
            "type: free\n"
            "allocator: \"mimalloc::aligned_free\"\n"
            "address: {}",
            p);
#endif
  mi_free_aligned(p, alignment);
}

void surge::allocators::mimalloc::init() {
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

  mi_option_set(mi_option_reserve_huge_os_pages, 1);
  mi_option_set(mi_option_eager_commit_delay, 100);
}

surge::allocators::mimalloc::arena::arena(usize cap) : data(cap, std::byte{0}), capacity{cap} {}

void surge::allocators::mimalloc::arena::reset() {
  log_info("Arena reset");
  offset = 0;
}

auto surge::allocators::mimalloc::arena::size() const -> usize { return offset; }

static auto is_pow_2(surge::usize x) { return (x & (x - 1)) == 0; }

auto surge::allocators::mimalloc::arena::allocate(usize size, usize alignment) -> void * {
  using std::memset;

  if (!is_pow_2(alignment)) {
    log_error("Alignment {} is not a power of 2", alignment);
    return nullptr;
  }

  const auto modulo{size & (alignment - 1)}; // same as size % alignment when a is a power of 2
  const auto actual_size{size + modulo};
  const auto new_offset{offset + actual_size};

  if (new_offset >= capacity) {
    log_warn("Unable to allocate {} B with {} B alignment. Arena is full", size, alignment);
    return nullptr;
  }

  auto p{static_cast<void *>(&(data[offset]))};
  offset = new_offset;
  memset(p, 0, actual_size);

#ifdef SURGE_DEBUG_MEMORY
  log_debug("Memory Event\n"
            "---\n"
            "type: alloc\n"
            "allocator: \"mimalloc::arena\"\n"
            "size: {}\n"
            "alignment: {}\n"
            "total aligned size: {}\n",
            "address: {}\n"
            "failed: {}",
            size, alignment, actual_size, p, p ? "false" : "true");
#endif

  return p;
}
