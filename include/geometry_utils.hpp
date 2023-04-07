#ifndef SURGE_GEOMETRY_UTILS_HPP
#define SURGE_GEOMETRY_UTILS_HPP

#include <glm/vec3.hpp>

namespace surge {

enum class actor_heading : std::ptrdiff_t {
  north = 0,
  south = 1,
  east = 2,
  west = 3,
  north_east = 4,
  north_west = 5,
  south_east = 6,
  south_west = 7,
  none = 8
};

[[nodiscard]] auto compute_heading_to(const glm::vec3 &vector) noexcept -> actor_heading;
[[nodiscard]] auto compute_heading_to(const glm::vec3 &start, const glm::vec3 &end) noexcept
    -> actor_heading;

} // namespace surge

#endif // SURGE_GEOMETRY_UTILS_HPP