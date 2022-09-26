#include "allocators/allocator_utils.hpp"

#include "options.hpp"

#ifdef SURGE_SYSTEM_IS_POSIX
#  include <unistd.h>
#endif

auto surge::is_pow_2(std::size_t x) -> bool { return (x & (x - 1)) == 0; }

auto surge::align_alloc_size(std::size_t intended_size, std::size_t alignment) -> std::size_t {

  std::size_t modulo{0};

  if (is_pow_2(alignment)) {
    modulo = intended_size & (alignment - 1);
  } else {
    modulo = intended_size % alignment;
  }

  return intended_size + (alignment - modulo);
}

auto surge::get_page_size() -> long {
#ifdef SURGE_SYSTEM_IS_POSIX
  return sysconf(_SC_PAGESIZE);
#endif
}