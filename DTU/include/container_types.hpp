#ifndef SURGE_MODULE_DTU_CONTAINER_TYPES_HPP
#define SURGE_MODULE_DTU_CONTAINER_TYPES_HPP

#include "player/allocators.hpp"

#include <foonathan/memory/container.hpp>
#include <foonathan/memory/std_allocator.hpp>
#include <string>

namespace DTU {

// Allocators
using mfa = surge::allocators::mimalloc::fnm_allocator;
template <typename T> using std_mfa = foonathan::memory::std_allocator<T, mfa>;

// Container Aliases
template <typename T> using deque = foonathan::memory::deque<T, mfa>;
using string = std::basic_string<char, std::char_traits<char>, std_mfa<char>>;

} // namespace DTU

#endif // SURGE_MODULE_DTU_CONTAINER_TYPES_HPP