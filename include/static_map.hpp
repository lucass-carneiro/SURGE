#ifndef SURGE_STATIC_MAP_HPP
#define SURGE_STATIC_MAP_HPP

#include <array>
#include <cstddef>

namespace surge {

template <typename key_t, typename value_t, std::size_t size> class static_map {
public:
  struct element {
    key_t key;
    value_t value;
  };

  constexpr auto operator[](key_t key) const noexcept -> value_t {
    return get(key);
  }

private:
  constexpr auto get(key_t key, std::size_t i = 0) const noexcept -> value_t {
    return i == size             ? key_not_found()
           : pairs[i].key == key ? pairs[i].value
                                 : get(key, i + 1);
  }

  static auto key_not_found() noexcept -> value_t { return value_t{}; }

public:
  std::array<element, size> pairs;
};

} // namespace surge

#endif // SURGE_STATIC_MAP_HPP