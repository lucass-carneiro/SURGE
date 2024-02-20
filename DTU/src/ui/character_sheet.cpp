// clang-format off
#include "ui/character_sheet.hpp"

#include "player/logging.hpp"

#include <cmath>
#include <cstdio>
#include <array>
#include <string>
// clang-format on

namespace color {
static constexpr glm::vec4 white{1.0f, 1.0f, 1.0f, 1.0f};
}

namespace geometry {

static constexpr glm::vec3 baseline_skip{7.0f, 27.0f, 0.0f};

// clang-format off
static constexpr auto empathy_text_baseline{glm::vec3{170.285f, 152.916f, 0.1f} + baseline_skip};
static constexpr auto introspection_text_baseline{glm::vec3{426.556f, 152.916f, 0.1f} + baseline_skip};
static constexpr auto reasoning_text_baseline{glm::vec3{172.285f, 219.538f, 0.1f} + baseline_skip};
static constexpr auto linguistics_text_baseline{glm::vec3{426.556f, 219.538f, 0.1f} + baseline_skip};
static constexpr auto fitness_text_baseline{glm::vec3{172.285f, 282.518f, 0.1f} + baseline_skip};
static constexpr auto agility_text_baseline{glm::vec3{426.556f, 282.518, 0.1f} + baseline_skip};
// clang-format on

static constexpr auto points_text_baseline{glm::vec3{363.968f, 350.0f, 0.1f} + baseline_skip};

static constexpr auto help_text_baseline{glm::vec3{542.092f, 569.588f, 0.1f} + baseline_skip};

static constexpr glm::vec4 empathy_area{29.466, 152.916, 199.319, 36.322};
static constexpr glm::vec4 empathy_bttn{212.642, 152.916f, 16.143, 36.322};
static constexpr glm::vec4 empathy_bttn_up{212.642, 152.916f, 16.143, 36.322 / 2.0};
static constexpr glm::vec4 empathy_bttn_down{212.642, 152.916f + 36.322 / 2.0, 16.143,
                                             36.322 / 2.0};

static constexpr glm::vec4 introspection_area{233.784, 152.916, 249.272, 36.322};
static constexpr glm::vec4 introspection_bttn{466.913, 152.916f, 16.143, 36.322};
static constexpr glm::vec4 introspection_bttn_up{466.913, 152.916f, 16.143, 36.322 / 2.0};
static constexpr glm::vec4 introspection_bttn_down{466.913, 152.916f + 36.322 / 2.0, 16.143,
                                                   36.322 / 2.0};

static constexpr glm::vec4 reasoning_area{18, 219.538, 210.785, 36.322};
static constexpr glm::vec4 reasoning_bttn{212.642, 219.538, 16.143, 36.322};
static constexpr glm::vec4 reasoning_bttn_up{212.642, 219.538, 16.143, 36.322 / 2.0};
static constexpr glm::vec4 reasoning_bttn_down{212.642, 219.538 + 36.322 / 2.0, 16.143,
                                               36.322 / 2.0};

static constexpr glm::vec4 linguistics_area{256.108, 219.538, 226.947, 36.322};
static constexpr glm::vec4 linguistics_bttn{466.913, 219.538, 16.143, 36.322};
static constexpr glm::vec4 linguistics_bttn_up{466.913, 219.538, 16.143, 36.322 / 2.0};
static constexpr glm::vec4 linguistics_bttn_down{466.913, 219.538 + 36.322 / 2.0, 16.143,
                                                 36.322 / 2.0};

static constexpr glm::vec4 fitness_area{43.026, 282.518, 185.759, 32.321};
static constexpr glm::vec4 fitness_bttn{212.642, 282.518, 16.143, 32.322};
static constexpr glm::vec4 fitness_bttn_up{212.642, 282.518, 16.143, 32.322 / 2.0};
static constexpr glm::vec4 fitness_bttn_down{212.642, 282.518 + 32.322 / 2.0, 16.143, 32.322 / 2.0};

static constexpr glm::vec4 agility_area{286.819, 285.518, 196.237, 36.321};
static constexpr glm::vec4 agility_bttn{466.913, 282.518, 16.143, 36.322};
static constexpr glm::vec4 agility_bttn_up{466.913, 282.518, 16.143, 36.322 / 2.0};
static constexpr glm::vec4 agility_bttn_down{466.913, 282.518 + 36.322 / 2.0, 16.143, 36.322 / 2.0};

static constexpr glm::vec4 reset_bttn_rect{128.153, 698, 86.063, 57.376};

static constexpr glm::vec4 health_points_area{128.153, 525.604, 243.695, 36.321};
static constexpr auto health_points_value{glm::vec3{335.526, 525.604, 0.1} + baseline_skip};

static constexpr glm::vec4 actions_points_area{128.153, 567.701, 243.695, 36.321};
static constexpr auto action_points_value{glm::vec3{335.526, 576.701, 0.1} + baseline_skip};

static constexpr glm::vec4 psyche_points_area{128.153, 612.973, 243.695, 36.321};
static constexpr auto psyche_points_value{glm::vec3{335.526, 612.973, 0.1} + baseline_skip};

static constexpr glm::vec4 initiative_points_area{128.153, 658.260, 243.695, 36.321};
static constexpr auto initiative_points_value{glm::vec3{335.526, 658.260, 0.1} + baseline_skip};

} // namespace geometry

namespace elements {

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

} // namespace elements

struct ui_state_desc {
  glm::vec3 background_scale{1.0f};

  // Attributes
  elements::u8_text empathy{0, geometry::empathy_text_baseline, color::white};
  elements::u8_text introspection{0, geometry::introspection_text_baseline, color::white};
  elements::u8_text reasoning{0, geometry::reasoning_text_baseline, color::white};
  elements::u8_text linguistics{0, geometry::linguistics_text_baseline, color::white};
  elements::u8_text fitness{0, geometry::fitness_text_baseline, color::white};
  elements::u8_text agility{0, geometry::agility_text_baseline, color::white};
  elements::u8_text points{12, geometry::points_text_baseline, color::white};

  elements::u8_text hp{2, geometry::health_points_value, color::white};
  elements::u8_text ap{2, geometry::action_points_value, color::white};
  elements::u8_text pp{2, geometry::psyche_points_value, color::white};
  elements::u8_text in{0, geometry::initiative_points_value, color::white};

  // Help text
  elements::text help{"", geometry::help_text_baseline, color::white};
};

struct element_handles {
  GLuint64 bckg{0};
  GLuint64 d4{0};
  GLuint64 d6{0};
  GLuint64 d8{0};
  GLuint64 d10{0};
  GLuint64 d12{0};
  GLuint64 d20{0};
  GLuint64 drop{0};
};

static element_handles g_elm_handles{}; // NOLINT

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

void DTU::ui::character_sheet::load(vec_glui &ids, vec_glui64 &handles) noexcept {
  g_elm_handles.bckg = DTU::load_texture(ids, handles, "resources/ui/character_sheet/base.png");
  g_elm_handles.d4 = DTU::load_texture(ids, handles, "resources/ui/character_sheet/d4.png");
  g_elm_handles.d6 = DTU::load_texture(ids, handles, "resources/ui/character_sheet/d6.png");
  g_elm_handles.d8 = DTU::load_texture(ids, handles, "resources/ui/character_sheet/d8.png");
  g_elm_handles.d10 = DTU::load_texture(ids, handles, "resources/ui/character_sheet/d10.png");
  g_elm_handles.d12 = DTU::load_texture(ids, handles, "resources/ui/character_sheet/d12.png");
  g_elm_handles.d20 = DTU::load_texture(ids, handles, "resources/ui/character_sheet/d20.png");
  g_elm_handles.drop = DTU::load_texture(ids, handles, "resources/ui/character_sheet/drop.png");

  surge::atom::sprite::make_resident(handles);
}

static inline void empathy_hoover(DTU::cmdq_t &cmdq, ui_state_desc &current_state,
                                  const glm::vec2 &cursor_pos) noexcept {
  using namespace DTU;

  // Cursor position of previous call
  static glm::vec2 prev_cp{0.0f};

  // Enter
  if (point_in_rect(cursor_pos, geometry::empathy_area)
      && !point_in_rect(prev_cp, geometry::empathy_area)) {
    current_state.help.text = "Your ability to understand, de-\n"
                              "tect and empathize with the\n"
                              "feelings of others.";
    cmdq.push_back(commands::ui_refresh);
  }

  // Exit
  if (!point_in_rect(cursor_pos, geometry::empathy_area)
      && point_in_rect(prev_cp, geometry::empathy_area)) {
    current_state.help.text.clear();
    cmdq.push_back(commands::ui_refresh);
  }

  prev_cp = cursor_pos;
}

static inline void introspection_hoover(DTU::cmdq_t &cmdq, ui_state_desc &current_state,
                                        const glm::vec2 &cursor_pos) noexcept {
  using namespace DTU;

  // Cursor position of previous call
  static glm::vec2 prev_cp{0.0f};

  // Enter
  if (point_in_rect(cursor_pos, geometry::introspection_area)
      && !point_in_rect(prev_cp, geometry::introspection_area)) {
    current_state.help.text = "Your ability to understand\n"
                              "yourself, your desires\n"
                              "fears and overall mental\n"
                              "state.";
    cmdq.push_back(commands::ui_refresh);
  }

  // Exit
  if (!point_in_rect(cursor_pos, geometry::introspection_area)
      && point_in_rect(prev_cp, geometry::introspection_area)) {
    current_state.help.text.clear();
    cmdq.push_back(commands::ui_refresh);
  }

  prev_cp = cursor_pos;
}

static inline void reasoning_hoover(DTU::cmdq_t &cmdq, ui_state_desc &current_state,
                                    const glm::vec2 &cursor_pos) noexcept {
  using namespace DTU;

  // Cursor position of previous call
  static glm::vec2 prev_cp{0.0f};

  // Enter
  if (point_in_rect(cursor_pos, geometry::reasoning_area)
      && !point_in_rect(prev_cp, geometry::reasoning_area)) {
    current_state.help.text = "Your ability to apply logic\n"
                              "and abstract thinking to\n"
                              "solve problems.";
    cmdq.push_back(commands::ui_refresh);
  }

  // Exit
  if (!point_in_rect(cursor_pos, geometry::reasoning_area)
      && point_in_rect(prev_cp, geometry::reasoning_area)) {
    current_state.help.text.clear();
    cmdq.push_back(commands::ui_refresh);
  }

  prev_cp = cursor_pos;
}

static inline void linguistics_hoover(DTU::cmdq_t &cmdq, ui_state_desc &current_state,
                                      const glm::vec2 &cursor_pos) noexcept {
  using namespace DTU;

  // Cursor position of previous call
  static glm::vec2 prev_cp{0.0f};

  // Enter
  if (point_in_rect(cursor_pos, geometry::linguistics_area)
      && !point_in_rect(prev_cp, geometry::linguistics_area)) {
    current_state.help.text = "Your ability to learn, express\n"
                              "yourself and understand diff-\n"
                              "erent languages and forms of\n"
                              "communication.";
    cmdq.push_back(commands::ui_refresh);
  }

  // Exit
  if (!point_in_rect(cursor_pos, geometry::linguistics_area)
      && point_in_rect(prev_cp, geometry::linguistics_area)) {
    current_state.help.text.clear();
    cmdq.push_back(commands::ui_refresh);
  }

  prev_cp = cursor_pos;
}

static inline void fitness_hoover(DTU::cmdq_t &cmdq, ui_state_desc &current_state,
                                  const glm::vec2 &cursor_pos) noexcept {
  using namespace DTU;

  // Cursor position of previous call
  static glm::vec2 prev_cp{0.0f};

  // Enter
  if (point_in_rect(cursor_pos, geometry::fitness_area)
      && !point_in_rect(prev_cp, geometry::fitness_area)) {
    current_state.help.text = "Your overall physical fitness\n"
                              "including bodily health, mus-\n"
                              "cle mass and stamina.";
    cmdq.push_back(commands::ui_refresh);
  }

  // Exit
  if (!point_in_rect(cursor_pos, geometry::fitness_area)
      && point_in_rect(prev_cp, geometry::fitness_area)) {
    current_state.help.text.clear();
    cmdq.push_back(commands::ui_refresh);
  }

  prev_cp = cursor_pos;
}

static inline void agility_hoover(DTU::cmdq_t &cmdq, ui_state_desc &current_state,
                                  const glm::vec2 &cursor_pos) noexcept {
  using namespace DTU;

  // Cursor position of previous call
  static glm::vec2 prev_cp{0.0f};

  // Enter
  if (point_in_rect(cursor_pos, geometry::agility_area)
      && !point_in_rect(prev_cp, geometry::agility_area)) {
    current_state.help.text = "Your ability to perform com-\n"
                              "plex and precise bodily mo-\n"
                              "tions and actions.";
    cmdq.push_back(commands::ui_refresh);
  }

  // Exit
  if (!point_in_rect(cursor_pos, geometry::agility_area)
      && point_in_rect(prev_cp, geometry::agility_area)) {
    current_state.help.text.clear();
    cmdq.push_back(commands::ui_refresh);
  }

  prev_cp = cursor_pos;
}

static inline void health_points_hoover(DTU::cmdq_t &cmdq, ui_state_desc &current_state,
                                        const glm::vec2 &cursor_pos) noexcept {
  using namespace DTU;

  // Cursor position of previous call
  static glm::vec2 prev_cp{0.0f};

  // Enter
  if (point_in_rect(cursor_pos, geometry::health_points_area)
      && !point_in_rect(prev_cp, geometry::health_points_area)) {
    current_state.help.text = "Represents the character's\n"
                              "overall physical health.";
    cmdq.push_back(commands::ui_refresh);
  }

  // Exit
  if (!point_in_rect(cursor_pos, geometry::health_points_area)
      && point_in_rect(prev_cp, geometry::health_points_area)) {
    current_state.help.text.clear();
    cmdq.push_back(commands::ui_refresh);
  }

  prev_cp = cursor_pos;
}

static inline void action_points_hoover(DTU::cmdq_t &cmdq, ui_state_desc &current_state,
                                        const glm::vec2 &cursor_pos) noexcept {
  using namespace DTU;

  // Cursor position of previous call
  static glm::vec2 prev_cp{0.0f};

  // Enter
  if (point_in_rect(cursor_pos, geometry::actions_points_area)
      && !point_in_rect(prev_cp, geometry::actions_points_area)) {
    current_state.help.text = "Represents a character's\n"
                              "overall mental health.";
    cmdq.push_back(commands::ui_refresh);
  }

  // Exit
  if (!point_in_rect(cursor_pos, geometry::actions_points_area)
      && point_in_rect(prev_cp, geometry::actions_points_area)) {
    current_state.help.text.clear();
    cmdq.push_back(commands::ui_refresh);
  }

  prev_cp = cursor_pos;
}

static inline void psyche_points_hoover(DTU::cmdq_t &cmdq, ui_state_desc &current_state,
                                        const glm::vec2 &cursor_pos) noexcept {
  using namespace DTU;

  // Cursor position of previous call
  static glm::vec2 prev_cp{0.0f};

  // Enter
  if (point_in_rect(cursor_pos, geometry::psyche_points_area)
      && !point_in_rect(prev_cp, geometry::psyche_points_area)) {
    current_state.help.text = "Represents a character's\n"
                              "actions and proactivity\n"
                              "during combat scenarios.";
    cmdq.push_back(commands::ui_refresh);
  }

  // Exit
  if (!point_in_rect(cursor_pos, geometry::psyche_points_area)
      && point_in_rect(prev_cp, geometry::psyche_points_area)) {
    current_state.help.text.clear();
    cmdq.push_back(commands::ui_refresh);
  }

  prev_cp = cursor_pos;
}

static inline void initiative_points_hoover(DTU::cmdq_t &cmdq, ui_state_desc &current_state,
                                            const glm::vec2 &cursor_pos) noexcept {
  using namespace DTU;

  // Cursor position of previous call
  static glm::vec2 prev_cp{0.0f};

  // Enter
  if (point_in_rect(cursor_pos, geometry::initiative_points_area)
      && !point_in_rect(prev_cp, geometry::initiative_points_area)) {
    current_state.help.text = "Represents a character's\n"
                              "readiness for combat\n"
                              "encounters.";
    cmdq.push_back(commands::ui_refresh);
  }

  // Exit
  if (!point_in_rect(cursor_pos, geometry::initiative_points_area)
      && point_in_rect(prev_cp, geometry::initiative_points_area)) {
    current_state.help.text.clear();
    cmdq.push_back(commands::ui_refresh);
  }

  prev_cp = cursor_pos;
}

static inline void update_state(DTU::cmdq_t &cmdq, ui_state_desc &current_state,
                                const glm::vec2 &window_dims,
                                const glm::vec2 &cursor_pos) noexcept {
  using namespace DTU;
  using std::abs;
  using std::ceil;

  // Handle window resizes
  const auto delta_w{abs(current_state.background_scale[0] - window_dims[0])};
  const auto delta_h{abs(current_state.background_scale[1] - window_dims[1])};

  if (delta_w > 1.0f || delta_h > 1.0f) {
    current_state.background_scale[0] = window_dims[0];
    current_state.background_scale[1] = window_dims[1];
    current_state.background_scale[2] = 1.0f;
    cmdq.push_back(commands::ui_refresh);
  }

  // Handle mouse hovering into specific areas
  empathy_hoover(cmdq, current_state, cursor_pos);
  introspection_hoover(cmdq, current_state, cursor_pos);
  reasoning_hoover(cmdq, current_state, cursor_pos);
  linguistics_hoover(cmdq, current_state, cursor_pos);
  fitness_hoover(cmdq, current_state, cursor_pos);
  agility_hoover(cmdq, current_state, cursor_pos);
  health_points_hoover(cmdq, current_state, cursor_pos);
  action_points_hoover(cmdq, current_state, cursor_pos);
  psyche_points_hoover(cmdq, current_state, cursor_pos);
  initiative_points_hoover(cmdq, current_state, cursor_pos);

  // Handle UI commands
  switch (cmdq.size() == 0 ? commands::idle : cmdq.front()) {

  case commands::empathy_up:
    if (current_state.empathy.value + 1 <= 5 && current_state.points.value - 1 >= 0) {
      current_state.empathy.value += 1;
      current_state.points.value -= 1;
      cmdq.push_back(commands::ui_refresh);
    }
    cmdq.pop_front();
    break;

  case commands::empathy_down:
    if (current_state.empathy.value - 1 >= 0) {
      current_state.empathy.value -= 1;
      current_state.points.value += 1;
      cmdq.push_back(commands::ui_refresh);
    }
    cmdq.pop_front();
    break;

  case commands::introspection_up:
    if (current_state.introspection.value + 1 <= 5 && current_state.points.value - 1 >= 0) {
      current_state.introspection.value += 1;
      current_state.points.value -= 1;
      current_state.pp.value = 2 * (current_state.introspection.value + 1);
      cmdq.push_back(commands::ui_refresh);
    }
    cmdq.pop_front();
    break;

  case commands::introspection_down:
    if (current_state.introspection.value - 1 >= 0) {
      current_state.introspection.value -= 1;
      current_state.points.value += 1;
      current_state.pp.value = 2 * (current_state.introspection.value + 1);
      cmdq.push_back(commands::ui_refresh);
    }
    cmdq.pop_front();
    break;

  case commands::reasoning_up:
    if (current_state.reasoning.value + 1 <= 5 && current_state.points.value - 1 >= 0) {
      current_state.reasoning.value += 1;
      current_state.points.value -= 1;
      cmdq.push_back(commands::ui_refresh);
    }
    cmdq.pop_front();
    break;

  case commands::reasoning_down:
    if (current_state.reasoning.value - 1 >= 0) {
      current_state.reasoning.value -= 1;
      current_state.points.value += 1;
      cmdq.push_back(commands::ui_refresh);
    }
    cmdq.pop_front();
    break;

  case commands::linguistics_up:
    if (current_state.linguistics.value + 1 <= 5 && current_state.points.value - 1 >= 0) {
      current_state.linguistics.value += 1;
      current_state.points.value -= 1;
      cmdq.push_back(commands::ui_refresh);
    }
    cmdq.pop_front();
    break;

  case commands::linguistics_down:
    if (current_state.linguistics.value - 1 >= 0) {
      current_state.linguistics.value -= 1;
      current_state.points.value += 1;
      cmdq.push_back(commands::ui_refresh);
    }
    cmdq.pop_front();
    break;

  case commands::fitness_up:
    if (current_state.fitness.value + 1 <= 5 && current_state.points.value - 1 >= 0) {
      current_state.fitness.value += 1;
      current_state.points.value -= 1;
      current_state.hp.value = 2 * (current_state.fitness.value + 1);
      current_state.in.value = static_cast<surge::u8>(
          ceil((current_state.fitness.value + current_state.agility.value) / 2));
      cmdq.push_back(commands::ui_refresh);
    }
    cmdq.pop_front();
    break;

  case commands::fitness_down:
    if (current_state.fitness.value - 1 >= 0) {
      current_state.fitness.value -= 1;
      current_state.points.value += 1;
      current_state.hp.value = 2 * (current_state.fitness.value + 1);
      current_state.in.value = static_cast<surge::u8>(
          ceil((current_state.fitness.value + current_state.agility.value) / 2));
      cmdq.push_back(commands::ui_refresh);
    }
    cmdq.pop_front();
    break;

  case commands::agility_up:
    if (current_state.agility.value + 1 <= 5 && current_state.points.value - 1 >= 0) {
      current_state.agility.value += 1;
      current_state.points.value -= 1;
      current_state.ap.value = 2 * (current_state.agility.value + 1);
      current_state.in.value = static_cast<surge::u8>(
          ceil((current_state.fitness.value + current_state.agility.value) / 2));
      cmdq.push_back(commands::ui_refresh);
    }
    cmdq.pop_front();
    break;

  case commands::agility_down:
    if (current_state.agility.value - 1 >= 0) {
      current_state.agility.value -= 1;
      current_state.points.value += 1;
      current_state.ap.value = 2 * (current_state.agility.value + 1);
      current_state.in.value = static_cast<surge::u8>(
          ceil((current_state.fitness.value + current_state.agility.value) / 2));
      cmdq.push_back(commands::ui_refresh);
    }
    cmdq.pop_front();
    break;

  case commands::reset_click:
    current_state.empathy.value = 0;
    current_state.introspection.value = 0;
    current_state.reasoning.value = 0;
    current_state.linguistics.value = 0;
    current_state.fitness.value = 0;
    current_state.agility.value = 0;

    current_state.points.value = 12;

    current_state.hp.value = 2;
    current_state.ap.value = 2;
    current_state.pp.value = 2;
    current_state.in.value = 0;

    cmdq.push_back(commands::ui_refresh);
    cmdq.pop_front();
    break;

  default:
    break;
  }
}

static inline void bake_and_send(DTU::cmdq_t &cmdq, const ui_state_desc &current_state,
                                 const DTU::sbd_t &ui_sbd, DTU::sdl_t &ui_sdl, DTU::tdd_t &tdd,
                                 DTU::tgl_t &tgd, const glm::vec2 &window_dims) noexcept {
  using namespace DTU;
  using std::snprintf;

  if (cmdq.size() != 0 && cmdq.front() == commands::ui_refresh) {

    DTU::clear_sprites(ui_sdl);
    DTU::clear_text(tdd);

    // Background
    push_sprite(ui_sdl, g_elm_handles.bckg,
                make_model(glm::vec3{0.0f}, glm::vec3{window_dims, 1.0f}), 1.0);

    // Attributes
    {
      std::array<char, 2> buffer{0, 0};

      snprintf(buffer.data(), 2, "%u", current_state.empathy.value);
      surge::atom::text::append_text_draw_data(
          tdd, tgd, buffer.data(), current_state.empathy.baseline, current_state.empathy.color);

      snprintf(buffer.data(), 2, "%u", current_state.introspection.value);
      surge::atom::text::append_text_draw_data(tdd, tgd, buffer.data(),
                                               current_state.introspection.baseline,
                                               current_state.introspection.color);

      snprintf(buffer.data(), 2, "%u", current_state.reasoning.value);
      surge::atom::text::append_text_draw_data(
          tdd, tgd, buffer.data(), current_state.reasoning.baseline, current_state.reasoning.color);

      snprintf(buffer.data(), 2, "%u", current_state.linguistics.value);
      surge::atom::text::append_text_draw_data(tdd, tgd, buffer.data(),
                                               current_state.linguistics.baseline,
                                               current_state.linguistics.color);

      snprintf(buffer.data(), 2, "%u", current_state.fitness.value);
      surge::atom::text::append_text_draw_data(
          tdd, tgd, buffer.data(), current_state.fitness.baseline, current_state.fitness.color);

      snprintf(buffer.data(), 2, "%u", current_state.agility.value);
      surge::atom::text::append_text_draw_data(
          tdd, tgd, buffer.data(), current_state.agility.baseline, current_state.agility.color);
    }

    // Remaining points
    {
      std::array<char, 3> buffer{0, 0, 0};

      snprintf(buffer.data(), 3, "%u", current_state.points.value);
      surge::atom::text::append_text_draw_data(
          tdd, tgd, buffer.data(), current_state.points.baseline, current_state.points.color);
    }

    // Health points
    {
      std::array<char, 3> buffer{0, 0, 0};

      snprintf(buffer.data(), 3, "%u", current_state.hp.value);
      surge::atom::text::append_text_draw_data(tdd, tgd, buffer.data(), current_state.hp.baseline,
                                               current_state.hp.color);
    }

    // Action points
    {
      std::array<char, 3> buffer{0, 0, 0};

      snprintf(buffer.data(), 3, "%u", current_state.ap.value);
      surge::atom::text::append_text_draw_data(tdd, tgd, buffer.data(), current_state.ap.baseline,
                                               current_state.ap.color);
    }

    // Psyche points
    {
      std::array<char, 3> buffer{0, 0, 0};

      snprintf(buffer.data(), 3, "%u", current_state.pp.value);
      surge::atom::text::append_text_draw_data(tdd, tgd, buffer.data(), current_state.pp.baseline,
                                               current_state.pp.color);
    }

    // Initiative points
    {
      std::array<char, 3> buffer{0, 0, 0};

      snprintf(buffer.data(), 3, "%u", current_state.in.value);
      surge::atom::text::append_text_draw_data(tdd, tgd, buffer.data(), current_state.in.baseline,
                                               current_state.in.color);
    }

    // Help text
    if (current_state.help.text.size() != 0) {
      surge::atom::text::append_text_draw_data(
          tdd, tgd, current_state.help.text, current_state.help.baseline, current_state.help.color);
    }

    surge::atom::sprite::send_buffers(ui_sbd, ui_sdl);

    cmdq.pop_front();
  }
}

void DTU::ui::character_sheet::update(cmdq_t &cmdq, const sbd_t &ui_sbd, sdl_t &ui_sdl, tdd_t &tdd,
                                      tgl_t &tgd, const glm::vec2 &window_dims,
                                      const glm::vec2 &cursor_pos) noexcept {

  static ui_state_desc current_state{};
  update_state(cmdq, current_state, window_dims, cursor_pos);
  bake_and_send(cmdq, current_state, ui_sbd, ui_sdl, tdd, tgd, window_dims);
}

void DTU::ui::character_sheet::mouse_left_click(cmdq_t &cmdq,
                                                const glm::vec2 &cursor_pos) noexcept {

  if (point_in_rect(cursor_pos, geometry::reset_bttn_rect)) {
    cmdq.push_back(commands::reset_click);
  }

  if (point_in_rect(cursor_pos, geometry::empathy_bttn_up)) {
    cmdq.push_back(commands::empathy_up);
  }

  if (point_in_rect(cursor_pos, geometry::empathy_bttn_down)) {
    cmdq.push_back(commands::empathy_down);
  }

  if (point_in_rect(cursor_pos, geometry::introspection_bttn_up)) {
    cmdq.push_back(commands::introspection_up);
  }

  if (point_in_rect(cursor_pos, geometry::introspection_bttn_down)) {
    cmdq.push_back(commands::introspection_down);
  }

  if (point_in_rect(cursor_pos, geometry::reasoning_bttn_up)) {
    cmdq.push_back(commands::reasoning_up);
  }

  if (point_in_rect(cursor_pos, geometry::reasoning_bttn_down)) {
    cmdq.push_back(commands::reasoning_down);
  }

  if (point_in_rect(cursor_pos, geometry::linguistics_bttn_up)) {
    cmdq.push_back(commands::linguistics_up);
  }

  if (point_in_rect(cursor_pos, geometry::linguistics_bttn_down)) {
    cmdq.push_back(commands::linguistics_down);
  }

  if (point_in_rect(cursor_pos, geometry::fitness_bttn_up)) {
    cmdq.push_back(commands::fitness_up);
  }

  if (point_in_rect(cursor_pos, geometry::fitness_bttn_down)) {
    cmdq.push_back(commands::fitness_down);
  }

  if (point_in_rect(cursor_pos, geometry::agility_bttn_up)) {
    cmdq.push_back(commands::agility_up);
  }

  if (point_in_rect(cursor_pos, geometry::agility_bttn_down)) {
    cmdq.push_back(commands::agility_down);
  }
}

void DTU::ui::character_sheet::mouse_scroll_up(cmdq_t &cmdq, const glm::vec2 &cursor_pos) noexcept {
  if (point_in_rect(cursor_pos, geometry::empathy_bttn)) {
    cmdq.push_back(commands::empathy_up);
  }

  if (point_in_rect(cursor_pos, geometry::introspection_bttn)) {
    cmdq.push_back(commands::introspection_up);
  }

  if (point_in_rect(cursor_pos, geometry::reasoning_bttn)) {
    cmdq.push_back(commands::reasoning_up);
  }

  if (point_in_rect(cursor_pos, geometry::linguistics_bttn)) {
    cmdq.push_back(commands::linguistics_up);
  }

  if (point_in_rect(cursor_pos, geometry::fitness_bttn)) {
    cmdq.push_back(commands::fitness_up);
  }

  if (point_in_rect(cursor_pos, geometry::agility_bttn)) {
    cmdq.push_back(commands::agility_up);
  }
}

void DTU::ui::character_sheet::mouse_scroll_down(cmdq_t &cmdq,
                                                 const glm::vec2 &cursor_pos) noexcept {
  if (point_in_rect(cursor_pos, geometry::empathy_bttn)) {
    cmdq.push_back(commands::empathy_down);
  }

  if (point_in_rect(cursor_pos, geometry::introspection_bttn)) {
    cmdq.push_back(commands::introspection_down);
  }

  if (point_in_rect(cursor_pos, geometry::reasoning_bttn)) {
    cmdq.push_back(commands::reasoning_down);
  }

  if (point_in_rect(cursor_pos, geometry::linguistics_bttn)) {
    cmdq.push_back(commands::linguistics_down);
  }

  if (point_in_rect(cursor_pos, geometry::fitness_bttn)) {
    cmdq.push_back(commands::fitness_down);
  }

  if (point_in_rect(cursor_pos, geometry::agility_bttn)) {
    cmdq.push_back(commands::agility_down);
  }
}