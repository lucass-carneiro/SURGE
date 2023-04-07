#include "geometry_utils.hpp"

#include <cmath>
#include <numbers>

auto surge::compute_heading_to(const glm::vec3 &vector) noexcept -> actor_heading {

  const auto phi{std::numbers::pi + std::atan2(vector[1], vector[0])};
  const auto dphi{std::numbers::pi / 8};

  if (dphi < phi && phi < 3 * dphi) {
    return actor_heading::north_west;
  } else if (3 * dphi < phi && phi < 5 * dphi) {
    return actor_heading::north;
  } else if (5 * dphi < phi && phi < 7 * dphi) {
    return actor_heading::north_east;
  } else if (7 * dphi < phi && phi < 9 * dphi) {
    return actor_heading::east;
  } else if (9 * dphi < phi && phi < 11 * dphi) {
    return actor_heading::south_east;
  } else if (11 * dphi < phi && phi < 13 * dphi) {
    return actor_heading::south;
  } else if (13 * dphi < phi && phi < 15 * dphi) {
    return actor_heading::south_west;
  } else {
    return actor_heading::west;
  }
}

auto surge::compute_heading_to(const glm::vec3 &start, const glm::vec3 &end) noexcept
    -> actor_heading {
  const auto vector{end - start};
  return compute_heading_to(vector);
}