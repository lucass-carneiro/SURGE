#include "ui.hpp"

#include <array>
#include <cmath>
#include <cstdio>

auto DTU::ui::spinner_box(GLFWwindow *window, surge::i32 id, surge::i32 &active, surge::i32 &hot,
                          sdl_t &ui_sdl, const glm::vec3 &draw_pos, const glm::vec3 &draw_scale,
                          const GLuint64 &neutral_handle, const GLuint64 &up_handle,
                          const GLuint64 &down_handle, float alpha, tdd_t &tdd,
                          const DTU::tgd_t &tgl, const glm::vec4 &t_color,
                          const glm::vec2 &mouse_pos, surge::u8 &value, surge::u8 min,
                          surge::u8 max) noexcept -> bool {

  using std::snprintf;

  const glm::vec4 rect{draw_pos[0], draw_pos[1], draw_scale[0], draw_scale[1]};
  const glm::vec4 up_rect{rect[0], rect[1], rect[2], rect[3] / 2.0f};
  const glm::vec4 down_rect{rect[0], rect[1] + rect[3] / 2.0f, rect[2], rect[3] / 2.0f};

  glm::vec3 text_baseline{draw_pos[0], draw_pos[1] + draw_scale[1], draw_pos[2] + 0.1f};
  const glm::vec2 text_scale{draw_scale[0] / 347.0f, draw_scale[1] / 124.0f};

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
  if (point_in_rect(mouse_pos, rect) && id != active) {
    hot = id;
  }

  // Determine if the press was up or down and push the correct skin sprite
  if (bttn_result && point_in_rect(mouse_pos, up_rect) && value + 1 <= max) {
    value += 1;
    push_sprite(ui_sdl, up_handle, make_model(draw_pos, draw_scale), alpha);
  } else if (bttn_result && point_in_rect(mouse_pos, down_rect) && value - 1 >= min) {
    value -= 1;
    push_sprite(ui_sdl, down_handle, make_model(draw_pos, draw_scale), alpha);
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