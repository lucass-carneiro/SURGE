#include "ui.hpp"

#include <array>
#include <cmath>
#include <cstdio>

auto DTU::ui::spinner_box(GLFWwindow *window, surge::i32 id, surge::i32 &active, surge::i32 &hot,
                          sdl_t &ui_sdl, const glm::vec3 &draw_pos, const glm::vec3 &draw_scale,
                          GLuint64 neutral_handle, GLuint64 up_handle, GLuint64 down_handle,
                          float alpha, tdd_t &tdd, const DTU::tgd_t &tgl, const glm::vec4 &t_color,
                          const glm::vec2 &mouse_pos, surge::u8 &pool, surge::u8 &value,
                          surge::u8 min, surge::u8 max) noexcept -> bool {

  using std::snprintf;

  // Fundamental skin metrics
  const float skin_width{347.0f};  // Total width of the skin in 300 dpi
  const float skin_height{124.0f}; // Total height of the skin in 300 dpi

  const float text_area_width{300.0f}; // Width of the text area in 300 dpi
  const float bttn_area_width{47.0f};  // Width of A SINGLE BUTTON in 300 dpi
  const float bttn_area_height{55.0f}; // Height of A SINGLE BUTTON in 300 dpi

  // Derved skin metrics
  const float text_area_width_ratio{text_area_width / skin_width};
  const float bttn_area_width_ratio{bttn_area_width / skin_width};
  const float bttn_area_height_ratio{bttn_area_height / skin_height};

  const glm::vec4 widget_rect{draw_pos[0], draw_pos[1], draw_scale[0], draw_scale[1]};

  const glm::vec4 bttn_up_rect{draw_pos[0] + draw_scale[0] * text_area_width_ratio, draw_pos[1],
                               draw_scale[0] * bttn_area_width_ratio,
                               draw_scale[1] * bttn_area_height_ratio};

  const glm::vec4 bttn_down_rect{
      draw_pos[0] + draw_scale[0] * text_area_width_ratio,
      (draw_pos[1] + draw_scale[1]) - draw_scale[1] * bttn_area_height_ratio,
      draw_scale[0] * bttn_area_width_ratio, draw_scale[1] * bttn_area_height_ratio};

  glm::vec3 text_baseline{draw_pos[0], draw_pos[1] + draw_scale[1], draw_pos[2] + 0.1f};
  const glm::vec2 text_scale{draw_scale[0] / skin_width, draw_scale[1] / skin_height};

  // This logic is a curtesey of Casey Muratory -  Immediate-Mode Graphical User Interfaces - 2005
  bool bttn_result{false};

  if (id == active) {
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) {
      if (id == hot) {
        bttn_result = true;
      }
      active = -1;
    }
  } else if (id == hot) {
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
      active = id;
    }
  }

  // "If there is an active item, it doesn't sets us as hot" - Casey
  if (point_in_rect(mouse_pos, widget_rect) && id != active) {
    hot = id;
  }

  // Change value up or down when the button is released while subtracting from or adding to the
  // pool. If the pool does not allow a size chenge, nothing happens with the values
  if (bttn_result && point_in_rect(mouse_pos, bttn_up_rect) && value + 1 <= max && pool != 0) {
    value += 1;
    pool -= 1;
  } else if (bttn_result && point_in_rect(mouse_pos, bttn_down_rect) && value - 1 >= min) {
    value -= 1;
    pool += 1;
  }

  // Display the up or down skin when the button is held
  if (id == active) {
    if (point_in_rect(mouse_pos, bttn_up_rect)) {
      push_sprite(ui_sdl, up_handle, make_model(draw_pos, draw_scale), alpha);
    } else if (point_in_rect(mouse_pos, bttn_down_rect)) {
      push_sprite(ui_sdl, down_handle, make_model(draw_pos, draw_scale), alpha);
    } else {
      push_sprite(ui_sdl, neutral_handle, make_model(draw_pos, draw_scale), alpha);
    }
  } else {
    push_sprite(ui_sdl, neutral_handle, make_model(draw_pos, draw_scale), alpha);
  }

  // If the number is small enough, shift it closer to the buttons
  if (value < 10) {
    text_baseline[0] += draw_scale[0] / 1.8f;
  } else if (value >= 10 & value <= 99) {
    text_baseline[0] += draw_scale[0] / 3.8f;
  }

  // Parse value as text and add it to draw list.
  std::array<char, 4> buffer{0, 0, 0, 0};
  snprintf(buffer.data(), 4, "%u", value);
  surge::atom::text::append_text_draw_data(tdd, tgl, buffer.data(), text_baseline, t_color,
                                           text_scale);

  return bttn_result;
}