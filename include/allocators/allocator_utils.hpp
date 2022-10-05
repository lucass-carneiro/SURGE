#ifndef SURGE_ALLOCATOR_UTILS_HPP
#define SURGE_ALLOCATOR_UTILS_HPP

#include "options.hpp"

#include <cstddef>

#ifdef SURGE_SYSTEM_IS_POSIX
#  include <unistd.h>
#endif
namespace surge {

constexpr auto is_pow_2(std::size_t x) noexcept -> bool { return (x & (x - 1)) == 0; }

constexpr auto align_alloc_size(std::size_t intended_size,
                                std::size_t alignment = alignof(std::max_align_t)) noexcept
    -> std::size_t {

  std::size_t modulo{0};

  if (is_pow_2(alignment)) {
    modulo = intended_size & (alignment - 1);
  } else {
    modulo = intended_size % alignment;
  }

  return intended_size + (alignment - modulo);
}

inline auto get_page_size() noexcept -> long {
#ifdef SURGE_SYSTEM_IS_POSIX
  return sysconf(_SC_PAGESIZE);
#endif
}

} // namespace surge

#endif // SURGE_ALLOCATOR_UTILS_HPP