#ifndef DTU_UI_HPP
#define DTU_UI_HPP

#include "player/container_types.hpp"

#include <glm/glm.hpp>

namespace DTU::ui {

struct u8_text {
  surge::u8 value;
  glm::vec3 baseline{0.0f};
  glm::vec4 color{1.0f};
};

struct text {
  surge::string text;
  glm::vec3 baseline{0.0f};
  glm::vec4 color{1.0f};
};

static inline auto point_in_rect(const glm::vec2 &point, const glm::vec4 &rect) noexcept {
  const auto x{point[0]};
  const auto y{point[1]};

  const auto x0{rect[0]};
  const auto xf{rect[0] + rect[2]};

  const auto y0{rect[1]};
  const auto yf{rect[1] + rect[3]};

  const auto in_x_range{x0 < x && x < xf};
  const auto in_y_range{y0 < y && y < yf};

  return in_x_range && in_y_range;
}

} // namespace DTU::ui

#endif // DTU_UI_HPP