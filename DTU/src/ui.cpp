#include "ui.hpp"

#include "player/logging.hpp"

auto DTU::ui::spinner_box(GLFWwindow *window, surge::i32 id, surge::i32 &active, surge::i32 &hot,
                          sdl_t &ui_sdl, const glm::vec3 &draw_pos, const glm::vec3 &draw_scale,
                          GLuint64 neutral_handle, GLuint64 up_handle, GLuint64 down_handle,
                          float alpha, tdd_t &tdd, const DTU::tgd_t &tgl, const glm::vec4 &t_color,
                          const glm::vec2 &mouse_pos, surge::u8 &pool, surge::u8 &value,
                          surge::u8 min, surge::u8 max) noexcept -> bool {

  using std::snprintf;

  // Fundamental skin metrics
  constexpr float skin_width{347.0f};  // Total width of the skin in 300 dpi
  constexpr float skin_height{124.0f}; // Total height of the skin in 300 dpi

  constexpr float text_area_width{300.0f}; // Width of the text area in 300 dpi
  constexpr float bttn_area_width{47.0f};  // Width of A SINGLE BUTTON in 300 dpi
  constexpr float bttn_area_height{55.0f}; // Height of A SINGLE BUTTON in 300 dpi

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

  const auto mouse_in_widget{point_in_rect(mouse_pos, widget_rect)};
  const auto mouse_in_up{point_in_rect(mouse_pos, bttn_up_rect)};
  const auto mouse_in_down{point_in_rect(mouse_pos, bttn_down_rect)};

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
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && mouse_in_widget) {
      active = id;
    }
  }

  // "If there is an active item, it doesn't sets us as hot" - Casey
  if (mouse_in_widget && id != active) {
    hot = id;
  }

  // Change value up or down when the button is released while subtracting from or adding to the
  // pool. If the pool does not allow a size chenge, nothing happens with the values
  if (bttn_result && mouse_in_up && value + 1 <= max && pool != 0) {
    value += 1;
    pool -= 1;
  } else if (bttn_result && mouse_in_down && value - 1 >= min) {
    value -= 1;
    pool += 1;
  }

  // Display the up or down skin when the button is held
  if (id == active) {
    if (mouse_in_up) {
      push_sprite(ui_sdl, up_handle, make_model(draw_pos, draw_scale), alpha);
    } else if (mouse_in_down) {
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

  surge::atom::text::append_text_draw_data(tdd, tgl, value, text_baseline, t_color, text_scale);

  return bttn_result;
}

auto DTU::ui::text_on_hot(surge::i32, const surge::i32 &target, const surge::i32 &hot, tdd_t &tdd,
                          const DTU::tgd_t &tgl, std::string_view text, const glm::vec3 &baseline,
                          const glm::vec4 &t_color, const glm::vec2 &scale) noexcept -> bool {
  if (target == hot) {
    surge::atom::text::append_text_draw_data(tdd, tgl, text, baseline, t_color, scale);
    return true;
  } else {
    return false;
  }
}

auto DTU::ui::button(GLFWwindow *window, surge::i32 id, surge::i32 &active, surge::i32 &hot,
                     sdl_t &ui_sdl, const glm::vec3 &draw_pos, const glm::vec3 &draw_scale,
                     GLuint64 up_handle, GLuint64 down_handle, float alpha, tdd_t &tdd,
                     const DTU::tgd_t &tgd, const glm::vec4 &t_color, const glm::vec2 &mouse_pos,
                     std::string_view text) noexcept -> bool {

  // Fundamental skin metrics
  constexpr float skin_w{175.575f};
  constexpr float skin_h{50.0f};
  constexpr float res_ratio{96.0f / 300.0f};

  const glm::vec4 widget_rect{draw_pos[0], draw_pos[1], draw_scale[0], draw_scale[1]};

  const glm::vec3 text_baseline{draw_pos[0] + 10.0f / skin_w * draw_scale[0],
                                draw_pos[1] + 42.0f / skin_h * draw_scale[1], draw_pos[2] + 0.1f};
  const glm::vec2 text_scale{res_ratio * draw_scale[0] / skin_w,
                             res_ratio * draw_scale[1] / skin_h};

  const auto mouse_in_widget{point_in_rect(mouse_pos, widget_rect)};

  bool bttn_result{false};

  if (id == active) {
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) {
      if (id == hot) {
        bttn_result = true;
      }
      active = -1;
    }
  } else if (id == hot) {
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && mouse_in_widget) {
      active = id;
    }
  }

  // "If there is an active item, it doesn't sets us as hot" - Casey
  if (mouse_in_widget && id != active) {
    hot = id;
  }

  // Display the up or down skin when the button is held
  if (id == active) {
    push_sprite(ui_sdl, down_handle, make_model(draw_pos, draw_scale), alpha);
    surge::atom::text::append_text_draw_data(tdd, tgd, text, text_baseline, t_color, text_scale);
  } else {
    push_sprite(ui_sdl, up_handle, make_model(draw_pos, draw_scale), alpha);
    surge::atom::text::append_text_draw_data(tdd, tgd, text, text_baseline, t_color, text_scale);
  }

  return bttn_result;
}

auto DTU::ui::button(GLFWwindow *window, surge::i32 id, surge::i32 &active, surge::i32 &hot,
                     sdl_t &ui_sdl, const glm::vec3 &draw_pos, const glm::vec3 &draw_scale,
                     GLuint64 up_handle, GLuint64 down_handle, float alpha,
                     const glm::vec2 &mouse_pos) noexcept -> bool {

  // Fundamental skin metrics
  const glm::vec4 widget_rect{draw_pos[0], draw_pos[1], draw_scale[0], draw_scale[1]};

  const auto mouse_in_widget{point_in_rect(mouse_pos, widget_rect)};

  bool bttn_result{false};

  if (id == active) {
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) {
      if (id == hot) {
        bttn_result = true;
      }
      active = -1;
    }
  } else if (id == hot) {
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && mouse_in_widget) {
      active = id;
    }
  }

  // "If there is an active item, it doesn't sets us as hot" - Casey
  if (mouse_in_widget && id != active) {
    hot = id;
  }

  // Display the up or down skin when the button is held
  if (id == active) {
    push_sprite(ui_sdl, down_handle, make_model(draw_pos, draw_scale), alpha);
  } else {
    push_sprite(ui_sdl, up_handle, make_model(draw_pos, draw_scale), alpha);
  }

  return bttn_result;
}