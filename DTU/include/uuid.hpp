#ifndef SURGE_MODULE_DTU_UUID_HPP
#define SURGE_MODULE_DTU_UUID_HPP

#include "integer_types.hpp"

#include <array>

namespace DTU {

struct uuid {
  std::array<u8, 16> id{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  uuid() noexcept;

  constexpr auto operator==(const uuid &rhs) noexcept -> bool;
};

} // namespace DTU

#endif // SURGE_MODULE_DTU_UUID_HPP