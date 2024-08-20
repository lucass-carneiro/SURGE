#include "allocators.hpp"

#include "logging.hpp"
#include "options.hpp"

#include <mimalloc.h>
#include <stdexcept>

#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo)) && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#endif

auto surge::allocators::mimalloc::malloc(usize size) noexcept -> void * {
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

auto surge::allocators::mimalloc::realloc(void *p, usize newsize) noexcept -> void * {
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

auto surge::allocators::mimalloc::calloc(usize count, usize size) noexcept -> void * {
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

void surge::allocators::mimalloc::free(void *p) noexcept {
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
                                                  usize alignment) noexcept -> void * {
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

void surge::allocators::mimalloc::aligned_free(void *p, usize alignment) noexcept {
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

void surge::allocators::mimalloc::init() noexcept {
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