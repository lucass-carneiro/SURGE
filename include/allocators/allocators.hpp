#ifndef SURGE_ALLOCATORS_HPP
#define SURGE_ALLOCATORS_HPP

#include "default_allocator.hpp"
#include "linear_arena_allocator.hpp"
#include "stack_allocator.hpp"

namespace surge {
template <typename T>
concept surge_allocator = std::is_same<T, default_allocator>::value || std::is_same<
    T, linear_arena_allocator>::value || std::is_same<T, stack_allocator>::value;
}

#endif // SURGE_ALLOCATORS_HPP
