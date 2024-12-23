#ifndef SURGE_CORE_CONTAINER_TYPES_HPP
#define SURGE_CORE_CONTAINER_TYPES_HPP

#include "sc_allocators.hpp"

#include <array>
#include <deque>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

namespace surge {

// Allocators
template <typename T> using cpp_mimalloc = surge::allocators::mimalloc::cpp_allocator<T>;

// Container Aliases
template <typename T> using vector = std::vector<T, cpp_mimalloc<T>>;
template <typename T> using deque = std::deque<T, cpp_mimalloc<T>>;
template <typename T> using queue = std::queue<T, deque<T>>;
template <typename T, std::size_t N> using array = std::array<T, N>;
using string = std::basic_string<char, std::char_traits<char>, cpp_mimalloc<char>>;

template <typename Key, typename Value> using hash_map
    = std::unordered_map<Key, Value, std::hash<Key>, std::equal_to<Key>,
                         cpp_mimalloc<std::pair<const Key, Value>>>;

} // namespace surge

#endif // SURGE_CORE_CONTAINER_TYPES_HPP