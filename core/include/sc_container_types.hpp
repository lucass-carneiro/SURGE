#ifndef SURGE_CORE_CONTAINER_TYPES_HPP
#define SURGE_CORE_CONTAINER_TYPES_HPP

#include "sc_allocators.hpp"

#include <array>
#include <deque>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

namespace surge::containers::mimalloc {
template <typename T> using allocator = surge::allocators::mimalloc::STLAllocator<T>;

template <typename T> using Vector = std::vector<T, allocator<T>>;
template <typename T> using Deque = std::deque<T, allocator<T>>;
template <typename T> using Queue = std::queue<T, Deque<T>>;
using String = std::basic_string<char, std::char_traits<char>, allocator<char>>;

template <typename Key, typename Value> using HashMap
    = std::unordered_map<Key, Value, std::hash<Key>, std::equal_to<Key>,
                         allocator<std::pair<const Key, Value>>>;

} // namespace surge::containers::mimalloc

#endif // SURGE_CORE_CONTAINER_TYPES_HPP