#ifndef SURGE_ALLOCATOR_UTILS_HPP
#define SURGE_ALLOCATOR_UTILS_HPP

#ifdef SURGE_SYSTEM_IS_POSIX
#  include <unistd.h>
#endif

#include <cstddef>

namespace surge {

/**
 * @brief Determines if a number is a power of two, using bit operations.
 *
 * @param x The number to test
 * @return true If the number is a power of 2.
 * @return false It the numbe is not a power of 2.
 */
[[nodiscard]] auto is_pow_2(std::size_t x) -> bool;

/**
 * @brief Modifies an allocation size to be aligned with the specified
 * alignment
 *
 * @param intended_size The size one wishes to allocate.
 * @param alignment The alignment of the allocation.
 * @return std::size_t The aligned allocation size.
 */
[[nodiscard]] auto align_alloc_size(std::size_t intended_size,
                                    std::size_t alignment = alignof(std::max_align_t))
    -> std::size_t;

[[nodiscard]] auto get_page_size() -> long;

} // namespace surge

#endif // SURGE_ALLOCATOR_UTILS_HPP