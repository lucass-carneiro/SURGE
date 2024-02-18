// clang-format off
#include "ui/character_sheet.hpp"

#include "player/logging.hpp"

#include <cmath>
#include <cstdio>
#include <array>
#include <string_view>
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

static constexpr auto points_text_baseline{glm::vec3{363.968f, 350.0f, 0.1f} + baseline_skip};

static constexpr auto help_text_baseline{glm::vec3{542.092f, 569.588f, 0.1f} + baseline_skip};
// clang-format on

} // namespace geometry

namespace elements {

struct u8_text {
  surge::u8 value;
  glm::vec3 baseline{0.0f};
  glm::vec4 color{1.0f};
};

struct text {
  std::string_view text{};
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

static void update_state(DTU::cmdq_t &cmdq, ui_state_desc &current_state, float ww,
                         float wh) noexcept {
  using namespace DTU;
  using std::abs;

  // Handle window resizes
  const auto delta_w{abs(current_state.background_scale[0] - ww)};
  const auto delta_h{abs(current_state.background_scale[1] - wh)};

  if (delta_w > 1.0f || delta_h > 1.0f) {
    current_state.background_scale[0] = ww;
    current_state.background_scale[1] = wh;
    current_state.background_scale[2] = 1.0f;
    cmdq.push_back(commands::ui_refresh);
    return;
  }

  // Handle UI commands
  switch (cmdq.size() == 0 ? commands::idle : cmdq.front()) {

  case commands::empathy_up:
    if (current_state.empathy.value + 1 <= 5) {
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

  default:
    break;
  }
}

static void bake_and_send(DTU::cmdq_t &cmdq, const ui_state_desc &current_state,
                          const DTU::sbd_t &ui_sbd, DTU::sdl_t &ui_sdl, DTU::tdd_t &tdd,
                          DTU::tgl_t &tgd, float ww, float wh) noexcept {
  using namespace DTU;
  using std::snprintf;

  if (cmdq.size() != 0 && cmdq.front() == commands::ui_refresh) {

    DTU::clear_sprites(ui_sdl);
    DTU::clear_text(tdd);

    // Background
    push_sprite(ui_sdl, g_elm_handles.bckg, make_model(glm::vec3{0.0f}, glm::vec3{ww, wh, 1.0f}),
                1.0);

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

    surge::atom::sprite::send_buffers(ui_sbd, ui_sdl);

    cmdq.pop_front();
  }
}

void DTU::ui::character_sheet::update(cmdq_t &cmdq, const sbd_t &ui_sbd, sdl_t &ui_sdl, tdd_t &tdd,
                                      tgl_t &tgd, float ww, float wh) noexcept {
  static ui_state_desc current_state{};
  update_state(cmdq, current_state, ww, wh);
  bake_and_send(cmdq, current_state, ui_sbd, ui_sdl, tdd, tgd, ww, wh);
}