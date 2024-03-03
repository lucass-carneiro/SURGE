#ifndef DTU_UI_HPP
#define DTU_UI_HPP

#include "DTU.hpp"

namespace DTU::ui {

/*
 * Detect if a point is inside a rectangle
 */
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

// clang-format off
auto spinner_box(
  GLFWwindow *window,
  surge::i32 id,
  surge::i32 &active,
  surge::i32 &hot,

  sdl_t &ui_sdl,
  const glm::vec3 &draw_pos,
  const glm::vec3 &draw_scale,
  
  GLuint64 neutral_handle,
  GLuint64 up_handle,
  GLuint64 down_handle,
  
  float alpha,

  tdd_t &tdd,
  const DTU::tgd_t &tgd,
  const glm::vec4 &t_color,

  const glm::vec2 &mouse_pos,

  surge::u8 &pool,
  surge::u8 &value,
  surge::u8 min,
  surge::u8 max
) noexcept -> bool;
// clang-format on

// Show text if an area in the screen if another widget becomes hot
auto text_on_hot(surge::i32 id, const surge::i32 &target, const surge::i32 &hot, tdd_t &tdd,
                 const DTU::tgd_t &tgd, std::string_view text, const glm::vec3 &baseline,
                 const glm::vec4 &t_color, const glm::vec2 &scale) noexcept -> bool;

// clang-format off
auto button(GLFWwindow *window,
  surge::i32 id,
  surge::i32 &active,
  surge::i32 &hot,

  sdl_t &ui_sdl,
  const glm::vec3 &draw_pos,
  const glm::vec3 &draw_scale,
  
  GLuint64 up_handle,
  GLuint64 down_handle,
  
  float alpha,

  tdd_t &tdd,
  const DTU::tgd_t &tgd,
  const glm::vec4 &t_color,

  const glm::vec2 &mouse_pos,
  
  std::string_view text
) noexcept -> bool;
// clang-format on

// clang-format off
auto button(GLFWwindow *window,
  surge::i32 id,
  surge::i32 &active,
  surge::i32 &hot,

  sdl_t &ui_sdl,
  const glm::vec3 &draw_pos,
  const glm::vec3 &draw_scale,
  
  GLuint64 up_handle,
  GLuint64 down_handle,
  
  float alpha,

  const glm::vec2 &mouse_pos
) noexcept -> bool;
// clang-format on

} // namespace DTU::ui

#endif // DTU_UI_HPP