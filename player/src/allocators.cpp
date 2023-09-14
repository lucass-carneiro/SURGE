// clang-format off
#include "options.hpp"
#include "allocators.hpp"

#include <mimalloc.h>
#include "override_new_delete.hpp"
// clang-format on

void surge::allocators::mimalloc::free(void *p) noexcept { mi_free(p); }

auto surge::allocators::mimalloc::malloc(size_t size) noexcept -> void * { return mi_malloc(size); }

auto surge::allocators::mimalloc::zalloc(size_t size) noexcept -> void * { return mi_zalloc(size); }

auto surge::allocators::mimalloc::calloc(size_t count, size_t size) noexcept -> void * {
  return mi_calloc(count, size);
}

auto surge::allocators::mimalloc::realloc(void *p, size_t newsize) noexcept -> void * {
  return mi_realloc(p, newsize);
}

auto surge::allocators::mimalloc::recalloc(void *p, size_t count, size_t size) noexcept -> void * {
  return mi_recalloc(p, count, size);
}

auto surge::allocators::mimalloc::expand(void *p, size_t newsize) noexcept -> void * {
  return mi_expand(p, newsize);
}

auto surge::allocators::mimalloc::mallocn(size_t count, size_t size) noexcept -> void * {
  return mi_mallocn(count, size);
}

auto surge::allocators::mimalloc::reallocn(void *p, size_t count, size_t size) noexcept -> void * {
  return mi_reallocn(p, count, size);
}

auto surge::allocators::mimalloc::reallocf(void *p, size_t newsize) noexcept -> void * {
  return mi_reallocf(p, newsize);
}

auto surge::allocators::mimalloc::strdup(const char *s) noexcept -> char * { return mi_strdup(s); }

auto surge::allocators::mimalloc::strndup(const char *s, size_t n) noexcept -> char * {
  return mi_strndup(s, n);
}

auto surge::allocators::mimalloc::realpath(const char *fname, char *resolved_name) noexcept
    -> char * {
  return mi_realpath(fname, resolved_name);
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
  mi_option_enable(mi_option_page_reset);

  mi_option_set(mi_option_reserve_huge_os_pages, 1);
  mi_option_set(mi_option_eager_commit_delay, 100);
}

auto surge::allocators::eastl::gp_allocator::allocate(size_t n, int) const noexcept -> void * {
#ifdef SURGE_ENABLE_TRACY
  void *p = mi_malloc(n);
  TracyAlloc(p, n);
  return p;
#else
  return mi_malloc(n);
#endif
}

auto surge::allocators::eastl::gp_allocator::allocate(size_t n, size_t alignment, size_t,
                                                      int) const noexcept -> void * {
#ifdef SURGE_ENABLE_TRACY
  void *p = mi_aligned_alloc(alignment, n);
  TracyAlloc(p, n);
  return p;
#else
  return mi_aligned_alloc(alignment, n);
#endif
}

void surge::allocators::eastl::gp_allocator::deallocate(void *p, size_t n) const noexcept {
#ifdef SURGE_ENABLE_TRACY
  TracyFree(p);
  mi_free_size(p, n);
#else
  mi_free_size(p, n);
#endif
}

auto surge::allocators::eastl::gp_allocator::get_name() const noexcept -> const char * {
  return "mimalloc backed general porpouse EASTL allocator";
}

void surge::allocators::eastl::gp_allocator::set_name(const char *) {}

auto surge::allocators::eastl::operator==(const gp_allocator &, const gp_allocator &) noexcept
    -> bool {
  return true;
}

auto surge::allocators::eastl::operator!=(const gp_allocator &, const gp_allocator &) noexcept
    -> bool {
  return false;
}