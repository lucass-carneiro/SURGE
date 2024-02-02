// clang-format off
#include "options.hpp"
#include "allocators.hpp"
#include "logging.hpp"

#include <mimalloc.h>
#include <stdexcept>
// clang-format on

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#endif

auto surge::allocators::mimalloc::malloc(usize size) noexcept -> void * {
  auto p{mi_malloc(size)};
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Memory Event\n"
            "---\n"
            "type: alloc\n"
            "allocator: \"mimalloc::malloc\"\n"
            "size: %zu\n"
            "address: %p\n"
            "failed: %s",
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
            "new size: %zu\n"
            "old address: %p\n"
            "new address: %p\n"
            "failed: %s",
            newsize, p, q, q ? "false" : "true");
#endif
  return q;
}

void surge::allocators::mimalloc::free(void *p) noexcept {
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Memory Event\n"
            "---\n"
            "type: free\n"
            "allocator: \"mimalloc::free\"\n"
            "address: %p",
            p);
#endif
  mi_free(p);
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

auto surge::allocators::mimalloc::fnm_allocator::allocate_node(usize size, usize alignment)
    -> void * {
  auto p{mi_aligned_alloc(alignment, size)};

#ifdef SURGE_DEBUG_MEMORY
  log_debug("Memory Event\n"
            "---\n"
            "type: alloc\n"
            "allocator: \"fnm_allocator\"\n"
            "size: %zu\n"
            "alignment: %zu\n"
            "address: %p\n"
            "failed: %s",
            size, alignment, p, p ? "false" : "true");
#endif

  if (!p) {
    log_error("Memory Event Failed\n"
              "---\n"
              "type: alloc\n"
              "allocator: \"fnm_allocator\"\n"
              "size: %zu\n"
              "alignment: %zu",
              size, alignment);

    throw std::runtime_error("fnm_allocator alloc failure");
  }

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  TracyAlloc(p, size);
#endif

  return p;
}

#ifdef SURGE_DEBUG_MEMORY
void surge::allocators::mimalloc::fnm_allocator::deallocate_node(void *p, usize size,
                                                                 usize alignment) noexcept {
#else
void surge::allocators::mimalloc::fnm_allocator::deallocate_node(void *p, usize,
                                                                 usize alignment) noexcept {
#endif
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Memory Event\n"
            "---\n"
            "type: free\n"
            "allocator: \"fnm_allocator\"\n"
            "size: %zu\n"
            "alignment: %zu\n"
            "address: %p",
            size, alignment, p);
#endif

  mi_free_aligned(p, alignment);

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  TracyFree(p);
#endif
}