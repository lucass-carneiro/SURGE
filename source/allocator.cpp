#include "allocator.hpp"

#include "options.hpp"

// clang-format off
#include <mimalloc-new-delete.h>
// clang-format on

void surge::init_mimalloc() noexcept {
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
  mi_option_enable(mi_option_page_reset);

  mi_option_set(mi_option_reserve_huge_os_pages, 1);
  mi_option_set(mi_option_eager_commit_delay, 100);
}

auto surge::eastl_allocator::allocate(size_t n, int) const noexcept -> void * {
  return mi_malloc(n);
}

auto surge::eastl_allocator::allocate(size_t n, size_t alignment, size_t, int) const noexcept
    -> void * {
  return mi_aligned_alloc(alignment, n);
}

void surge::eastl_allocator::deallocate(void *p, size_t n) const noexcept { mi_free_size(p, n); }

auto surge::eastl_allocator::get_name() const noexcept -> const char * {
  return "mimalloc backed EASTL allocator";
}

void surge::eastl_allocator::set_name(const char *) {}

auto surge::operator==(const eastl_allocator &, const eastl_allocator &) noexcept -> bool {
  return true;
}

auto surge::operator!=(const eastl_allocator &, const eastl_allocator &) noexcept -> bool {
  return false;
}