#ifndef SURGE_CORE_CONTAINER_TYPES_HPP
#define SURGE_CORE_CONTAINER_TYPES_HPP

#include "allocators.hpp"
#include "static_queue.hpp"

#include <foonathan/memory/container.hpp>
#include <foonathan/memory/memory_pool.hpp>
#include <foonathan/memory/std_allocator.hpp>
#include <string>

namespace surge {

// Allocators
using fnm_mimalloc = surge::allocators::mimalloc::fnm_allocator;
template <typename T> using fnm_mimalloc_std = foonathan::memory::std_allocator<T, fnm_mimalloc>;

// Container Aliases
template <typename T, std::size_t N> using array = std::array<T, N>;
template <typename T> using deque = foonathan::memory::deque<T, fnm_mimalloc>;
template <typename T> using queue = foonathan::memory::queue<T, fnm_mimalloc>;
template <typename T> using vector = foonathan::memory::vector<T, fnm_mimalloc>;
using string = std::basic_string<char, std::char_traits<char>, fnm_mimalloc_std<char>>;

template <typename Key, typename Value> using hash_map
    = foonathan::memory::unordered_map<Key, Value, fnm_mimalloc>;

} // namespace surge

#endif // SURGE_CORE_CONTAINER_TYPES_HPP