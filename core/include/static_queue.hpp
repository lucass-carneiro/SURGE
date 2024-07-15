#ifndef SURGE_CORE_STATIC_QUEUE_HPP
#define SURGE_CORE_STATIC_QUEUE_HPP

#include "integer_types.hpp"
#include "logging.hpp"
#include "options.hpp"

#include <array>

namespace surge {

template <typename T, usize capacity> class static_queue {
private:
  usize _front{0};
  usize _back{0};
  usize _size{0};
  std::array<T, capacity> data{};

#ifdef SURGE_BUILD_TYPE_Debug
  const char *queue_name{nullptr};
#endif

public:
  static_queue([[maybe_unused]] const char *name = "static queue",
               std::convertible_to<T> auto &&...initial_values)
#ifdef SURGE_BUILD_TYPE_Debug
      : queue_name{name}
#endif
  {
    data.fill(T{});
    for (const auto &p : std::initializer_list<T>{initial_values...}) {
      push(p);
    }
  }

  void push(T value) noexcept {
#ifdef SURGE_BUILD_TYPE_Debug
    if (_size >= capacity) {
      log_warn(
          "Unable to push value to static queue \"{}\": Capacity exceeded. Ignoring push request",
          queue_name);
      return;
    }
#endif
    data[_back] = value;
    _back = (_back + 1) % capacity;
    _size++;
  }

  auto pop() noexcept -> T {
#ifdef SURGE_BUILD_TYPE_Debug
    if (_size >= capacity) {
      log_warn("Unable to pop value from static queue \"{}\": Empty queue. Returning empty T",
               queue_name);
      return T{};
    }
#endif
    const auto value{data[_front]};
    _front = (_front + 1) % capacity;
    _size--;
    return value;
  }

  [[nodiscard]] inline auto front() const noexcept -> const T & { return data[_front]; }

  [[nodiscard]] inline auto size() const noexcept -> usize { return _size; }
  [[nodiscard]] inline auto empty() const noexcept -> bool { return _size == 0; }
};

} // namespace surge

#endif // SURGE_CORE_STATIC_QUEUE_HPP