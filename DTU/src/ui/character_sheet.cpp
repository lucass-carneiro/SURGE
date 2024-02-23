// clang-format off
#include "ui/ui.hpp"
#include "ui/character_sheet.hpp"
#include "ui/character_sheet_elements.hpp"

#include "player/logging.hpp"

#include <cmath>
#include <cstdio>
#include <array>
#include <string>
// clang-format on

using namespace DTU::ui::character_sheet;

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

static inline void empathy_hoover(DTU::cmdq_t &cmdq, state &current_state,
                                  const glm::vec2 &cursor_pos,
                                  const glm::vec2 &window_dims) noexcept {
  using namespace DTU;
  using namespace DTU::ui;

  // Cursor position of previous call
  static glm::vec2 prev_cp{0.0f};

  // Position factor (see bellow)
  const glm::vec4 pf{window_dims[0] / geometry::dw, window_dims[1] / geometry::dh,
                     window_dims[0] / geometry::dw, window_dims[1] / geometry::dh};

  // Enter
  if (point_in_rect(cursor_pos, geometry::e_rect * pf)
      && !point_in_rect(prev_cp, geometry::e_rect * pf)) {
    current_state.help.text = "Your ability to understand, de-\n"
                              "tect and empathize with the\n"
                              "feelings of others.";
    cmdq.push_back(commands::ui_refresh);
  }

  // Exit
  if (!point_in_rect(cursor_pos, geometry::e_rect) && point_in_rect(prev_cp, geometry::e_rect)) {
    current_state.help.text.clear();
    cmdq.push_back(commands::ui_refresh);
  }

  prev_cp = cursor_pos;
}

static inline void introspection_hoover(DTU::cmdq_t &cmdq, state &current_state,
                                        const glm::vec2 &cursor_pos) noexcept {
  using namespace DTU;
  using namespace DTU::ui;

  // Cursor position of previous call
  static glm::vec2 prev_cp{0.0f};

  // Enter
  if (point_in_rect(cursor_pos, geometry::i_rect) && !point_in_rect(prev_cp, geometry::i_rect)) {
    current_state.help.text = "Your ability to understand\n"
                              "yourself, your desires\n"
                              "fears and overall mental\n"
                              "state.";
    cmdq.push_back(commands::ui_refresh);
  }

  // Exit
  if (!point_in_rect(cursor_pos, geometry::i_rect) && point_in_rect(prev_cp, geometry::i_rect)) {
    current_state.help.text.clear();
    cmdq.push_back(commands::ui_refresh);
  }

  prev_cp = cursor_pos;
}

static inline void reasoning_hoover(DTU::cmdq_t &cmdq, state &current_state,
                                    const glm::vec2 &cursor_pos) noexcept {
  using namespace DTU;
  using namespace DTU::ui;

  // Cursor position of previous call
  static glm::vec2 prev_cp{0.0f};

  // Enter
  if (point_in_rect(cursor_pos, geometry::r_rect) && !point_in_rect(prev_cp, geometry::r_rect)) {
    current_state.help.text = "Your ability to apply logic\n"
                              "and abstract thinking to\n"
                              "solve problems.";
    cmdq.push_back(commands::ui_refresh);
  }

  // Exit
  if (!point_in_rect(cursor_pos, geometry::r_rect) && point_in_rect(prev_cp, geometry::r_rect)) {
    current_state.help.text.clear();
    cmdq.push_back(commands::ui_refresh);
  }

  prev_cp = cursor_pos;
}

static inline void linguistics_hoover(DTU::cmdq_t &cmdq, state &current_state,
                                      const glm::vec2 &cursor_pos) noexcept {
  using namespace DTU;
  using namespace DTU::ui;

  // Cursor position of previous call
  static glm::vec2 prev_cp{0.0f};

  // Enter
  if (point_in_rect(cursor_pos, geometry::l_rect) && !point_in_rect(prev_cp, geometry::l_rect)) {
    current_state.help.text = "Your ability to learn, express\n"
                              "yourself and understand diff-\n"
                              "erent languages and forms of\n"
                              "communication.";
    cmdq.push_back(commands::ui_refresh);
  }

  // Exit
  if (!point_in_rect(cursor_pos, geometry::l_rect) && point_in_rect(prev_cp, geometry::l_rect)) {
    current_state.help.text.clear();
    cmdq.push_back(commands::ui_refresh);
  }

  prev_cp = cursor_pos;
}

static inline void fitness_hoover(DTU::cmdq_t &cmdq, state &current_state,
                                  const glm::vec2 &cursor_pos) noexcept {
  using namespace DTU;
  using namespace DTU::ui;

  // Cursor position of previous call
  static glm::vec2 prev_cp{0.0f};

  // Enter
  if (point_in_rect(cursor_pos, geometry::f_rect) && !point_in_rect(prev_cp, geometry::f_rect)) {
    current_state.help.text = "Your overall physical fitness\n"
                              "including bodily health, mus-\n"
                              "cle mass and stamina.";
    cmdq.push_back(commands::ui_refresh);
  }

  // Exit
  if (!point_in_rect(cursor_pos, geometry::f_rect) && point_in_rect(prev_cp, geometry::f_rect)) {
    current_state.help.text.clear();
    cmdq.push_back(commands::ui_refresh);
  }

  prev_cp = cursor_pos;
}

static inline void agility_hoover(DTU::cmdq_t &cmdq, state &current_state,
                                  const glm::vec2 &cursor_pos) noexcept {
  using namespace DTU;
  using namespace DTU::ui;

  // Cursor position of previous call
  static glm::vec2 prev_cp{0.0f};

  // Enter
  if (point_in_rect(cursor_pos, geometry::a_rect) && !point_in_rect(prev_cp, geometry::a_rect)) {
    current_state.help.text = "Your ability to perform com-\n"
                              "plex and precise bodily mo-\n"
                              "tions and actions.";
    cmdq.push_back(commands::ui_refresh);
  }

  // Exit
  if (!point_in_rect(cursor_pos, geometry::a_rect) && point_in_rect(prev_cp, geometry::a_rect)) {
    current_state.help.text.clear();
    cmdq.push_back(commands::ui_refresh);
  }

  prev_cp = cursor_pos;
}

static inline void health_points_hoover(DTU::cmdq_t &cmdq, state &current_state,
                                        const glm::vec2 &cursor_pos) noexcept {
  using namespace DTU;
  using namespace DTU::ui;

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

static inline void action_points_hoover(DTU::cmdq_t &cmdq, state &current_state,
                                        const glm::vec2 &cursor_pos) noexcept {
  using namespace DTU;
  using namespace DTU::ui;

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

static inline void psyche_points_hoover(DTU::cmdq_t &cmdq, state &current_state,
                                        const glm::vec2 &cursor_pos) noexcept {
  using namespace DTU;
  using namespace DTU::ui;

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

static inline void initiative_points_hoover(DTU::cmdq_t &cmdq, state &current_state,
                                            const glm::vec2 &cursor_pos) noexcept {
  using namespace DTU;
  using namespace DTU::ui;

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

static inline void update_state(DTU::cmdq_t &cmdq, state &current_state,
                                const glm::vec2 &window_dims,
                                const glm::vec2 &cursor_pos) noexcept {
  using namespace DTU;
  using std::abs;
  using std::ceil;

  // Handle window resizes
  const auto delta_w{abs(current_state.scale[0] - window_dims[0])};
  const auto delta_h{abs(current_state.scale[1] - window_dims[1])};

  if (delta_w > 1.0f || delta_h > 1.0f) {
    current_state.scale[0] = window_dims[0];
    current_state.scale[1] = window_dims[1];
    current_state.scale[2] = 1.0f;
    cmdq.push_back(commands::ui_refresh);
  }

  // Handle mouse hovering into specific areas
  empathy_hoover(cmdq, current_state, cursor_pos, window_dims);
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

static inline void bake_and_send(DTU::cmdq_t &cmdq, const state &current_state,
                                 const DTU::sbd_t &ui_sbd, DTU::sdl_t &ui_sdl, DTU::tdd_t &tdd,
                                 DTU::tgl_t &tgd) noexcept {
  using namespace DTU;
  using std::snprintf;

  if (cmdq.size() != 0 && cmdq.front() == commands::ui_refresh) {

    DTU::clear_sprites(ui_sdl);
    DTU::clear_text(tdd);

    /* The positioning factor is the ratio between design and current resolution. It puts objects in
     * the correct position even if the window resolution is different than the design resolution.
     */
    const glm::vec2 pf{current_state.scale[0] / ui::character_sheet::geometry::dw,
                       current_state.scale[1] / ui::character_sheet::geometry::dh};
    const glm::vec3 pf3{pf, 1.0f};

    // Background
    push_sprite(ui_sdl, g_elm_handles.bckg, make_model(glm::vec3{0.0f}, current_state.scale), 1.0);

    // Attributes
    {
      std::array<char, 2> buffer{0, 0};

      snprintf(buffer.data(), 2, "%u", current_state.empathy.value);
      surge::atom::text::append_text_draw_data(tdd, tgd, buffer.data(),
                                               current_state.empathy.baseline * pf3,
                                               current_state.empathy.color, pf);

      snprintf(buffer.data(), 2, "%u", current_state.introspection.value);
      surge::atom::text::append_text_draw_data(tdd, tgd, buffer.data(),
                                               current_state.introspection.baseline * pf3,
                                               current_state.introspection.color, pf);

      snprintf(buffer.data(), 2, "%u", current_state.reasoning.value);
      surge::atom::text::append_text_draw_data(tdd, tgd, buffer.data(),
                                               current_state.reasoning.baseline * pf3,
                                               current_state.reasoning.color, pf);

      snprintf(buffer.data(), 2, "%u", current_state.linguistics.value);
      surge::atom::text::append_text_draw_data(tdd, tgd, buffer.data(),
                                               current_state.linguistics.baseline * pf3,
                                               current_state.linguistics.color, pf);

      snprintf(buffer.data(), 2, "%u", current_state.fitness.value);
      surge::atom::text::append_text_draw_data(tdd, tgd, buffer.data(),
                                               current_state.fitness.baseline * pf3,
                                               current_state.fitness.color, pf);

      snprintf(buffer.data(), 2, "%u", current_state.agility.value);
      surge::atom::text::append_text_draw_data(tdd, tgd, buffer.data(),
                                               current_state.agility.baseline * pf3,
                                               current_state.agility.color, pf);
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

void DTU::ui::character_sheet::load(vec_glui &ids, vec_glui64 &handles) noexcept {
  g_elm_handles.bckg = DTU::load_texture(ids, handles, "resources/ui/character_sheet/base.png",
                                         surge::renderer::texture_filtering::linear);

  g_elm_handles.d4 = DTU::load_texture(ids, handles, "resources/ui/character_sheet/d4.png");
  g_elm_handles.d6 = DTU::load_texture(ids, handles, "resources/ui/character_sheet/d6.png");
  g_elm_handles.d8 = DTU::load_texture(ids, handles, "resources/ui/character_sheet/d8.png");
  g_elm_handles.d10 = DTU::load_texture(ids, handles, "resources/ui/character_sheet/d10.png");
  g_elm_handles.d12 = DTU::load_texture(ids, handles, "resources/ui/character_sheet/d12.png");
  g_elm_handles.d20 = DTU::load_texture(ids, handles, "resources/ui/character_sheet/d20.png");
  g_elm_handles.drop = DTU::load_texture(ids, handles, "resources/ui/character_sheet/drop.png");

  surge::atom::sprite::make_resident(handles);
}

void DTU::ui::character_sheet::update(cmdq_t &cmdq, const sbd_t &ui_sbd, sdl_t &ui_sdl, tdd_t &tdd,
                                      tgl_t &tgd, const glm::vec2 &window_dims,
                                      const glm::vec2 &cursor_pos) noexcept {

  static state current_state{};
  update_state(cmdq, current_state, window_dims, cursor_pos);
  bake_and_send(cmdq, current_state, ui_sbd, ui_sdl, tdd, tgd);
}

void DTU::ui::character_sheet::mouse_left_click(cmdq_t &cmdq,
                                                const glm::vec2 &cursor_pos) noexcept {

  if (point_in_rect(cursor_pos, geometry::reset_bttn_rect)) {
    cmdq.push_back(commands::reset_click);
  }

  if (point_in_rect(cursor_pos, geometry::e_bttn_up_rect)) {
    cmdq.push_back(commands::empathy_up);
  }

  if (point_in_rect(cursor_pos, geometry::e_bttn_down_rect)) {
    cmdq.push_back(commands::empathy_down);
  }

  if (point_in_rect(cursor_pos, geometry::i_bttn_up_rect)) {
    cmdq.push_back(commands::introspection_up);
  }

  if (point_in_rect(cursor_pos, geometry::i_bttn_down_rect)) {
    cmdq.push_back(commands::introspection_down);
  }

  if (point_in_rect(cursor_pos, geometry::r_bttn_up_rect)) {
    cmdq.push_back(commands::reasoning_up);
  }

  if (point_in_rect(cursor_pos, geometry::r_bttn_down_rect)) {
    cmdq.push_back(commands::reasoning_down);
  }

  if (point_in_rect(cursor_pos, geometry::l_bttn_up_rect)) {
    cmdq.push_back(commands::linguistics_up);
  }

  if (point_in_rect(cursor_pos, geometry::l_bttn_down_rect)) {
    cmdq.push_back(commands::linguistics_down);
  }

  if (point_in_rect(cursor_pos, geometry::f_bttn_up_rect)) {
    cmdq.push_back(commands::fitness_up);
  }

  if (point_in_rect(cursor_pos, geometry::f_bttn_down_rect)) {
    cmdq.push_back(commands::fitness_down);
  }

  if (point_in_rect(cursor_pos, geometry::a_bttn_up_rect)) {
    cmdq.push_back(commands::agility_up);
  }

  if (point_in_rect(cursor_pos, geometry::a_bttn_down_rect)) {
    cmdq.push_back(commands::agility_down);
  }
}

void DTU::ui::character_sheet::mouse_scroll_up(cmdq_t &cmdq, const glm::vec2 &cursor_pos) noexcept {
  if (point_in_rect(cursor_pos, geometry::e_bttn_rect)) {
    cmdq.push_back(commands::empathy_up);
  }

  if (point_in_rect(cursor_pos, geometry::i_bttn_rect)) {
    cmdq.push_back(commands::introspection_up);
  }

  if (point_in_rect(cursor_pos, geometry::r_bttn_rect)) {
    cmdq.push_back(commands::reasoning_up);
  }

  if (point_in_rect(cursor_pos, geometry::l_bttn_rect)) {
    cmdq.push_back(commands::linguistics_up);
  }

  if (point_in_rect(cursor_pos, geometry::f_bttn_rect)) {
    cmdq.push_back(commands::fitness_up);
  }

  if (point_in_rect(cursor_pos, geometry::a_bttn_rect)) {
    cmdq.push_back(commands::agility_up);
  }
}

void DTU::ui::character_sheet::mouse_scroll_down(cmdq_t &cmdq,
                                                 const glm::vec2 &cursor_pos) noexcept {
  if (point_in_rect(cursor_pos, geometry::e_bttn_rect)) {
    cmdq.push_back(commands::empathy_down);
  }

  if (point_in_rect(cursor_pos, geometry::i_bttn_rect)) {
    cmdq.push_back(commands::introspection_down);
  }

  if (point_in_rect(cursor_pos, geometry::r_bttn_rect)) {
    cmdq.push_back(commands::reasoning_down);
  }

  if (point_in_rect(cursor_pos, geometry::l_bttn_rect)) {
    cmdq.push_back(commands::linguistics_down);
  }

  if (point_in_rect(cursor_pos, geometry::f_bttn_rect)) {
    cmdq.push_back(commands::fitness_down);
  }

  if (point_in_rect(cursor_pos, geometry::a_bttn_rect)) {
    cmdq.push_back(commands::agility_down);
  }
}