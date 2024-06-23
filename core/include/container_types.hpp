#ifndef SURGE_CORE_CONTAINER_TYPES_HPP
#define SURGE_CORE_CONTAINER_TYPES_HPP

#include "allocators.hpp"

#include <foonathan/memory/container.hpp>
#include <foonathan/memory/std_allocator.hpp>
#include <string>

namespace surge {

// Allocators
using mfa = surge::allocators::mimalloc::fnm_allocator;
template <typename T> using std_mfa = foonathan::memory::std_allocator<T, mfa>;

// Container Aliases
template <typename T, std::size_t N> using array = std::array<T, N>;
template <typename T> using deque = foonathan::memory::deque<T, mfa>;
template <typename T> using queue = foonathan::memory::queue<T, mfa>;
template <typename T> using vector = foonathan::memory::vector<T, mfa>;
using string = std::basic_string<char, std::char_traits<char>, std_mfa<char>>;

template <typename Key, typename Value> using hash_map
    = foonathan::memory::unordered_map<Key, Value, mfa>;

} // namespace surge

#endif // SURGE_CORE_CONTAINER_TYPES_HPP