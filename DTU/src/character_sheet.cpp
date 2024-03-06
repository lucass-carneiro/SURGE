
// clang-format off
#include "DTU.hpp"
#include "character_sheet.hpp"
#include "character.hpp"
#include "ui.hpp"

#include "player/logging.hpp"

#include <cmath>
// clang-format on

struct element_handles {
  GLuint64 page_0{0};
  GLuint64 page_1{0};

  GLuint64 spinner_box_neutral{0};
  GLuint64 spinner_box_up{0};
  GLuint64 spinner_box_down{0};

  GLuint64 reset_bttn_up{0};
  GLuint64 reset_bttn_down{0};

  GLuint64 next_page_bttn_up{0};
  GLuint64 next_page_bttn_down{0};

  GLuint64 prev_page_bttn_up{0};
  GLuint64 prev_page_bttn_down{0};
};

static element_handles g_elm_handles{}; // NOLINT

void DTU::ui::character_sheet::load(vec_glui &ids, vec_glui64 &handles) noexcept {
  g_elm_handles.page_0
      = DTU::load_texture(ids, handles, "resources/ui/character_sheet/character_sheet_pg_0.png",
                          surge::renderer::texture_filtering::anisotropic);

  g_elm_handles.page_1
      = DTU::load_texture(ids, handles, "resources/ui/character_sheet/character_sheet_pg_1.png",
                          surge::renderer::texture_filtering::anisotropic);

  g_elm_handles.spinner_box_neutral
      = DTU::load_texture(ids, handles, "resources/ui/character_sheet/spinner_box_neutral.png",
                          surge::renderer::texture_filtering::anisotropic);
  g_elm_handles.spinner_box_up
      = DTU::load_texture(ids, handles, "resources/ui/character_sheet/spinner_box_up.png",
                          surge::renderer::texture_filtering::anisotropic);
  g_elm_handles.spinner_box_down
      = DTU::load_texture(ids, handles, "resources/ui/character_sheet/spinner_box_down.png",
                          surge::renderer::texture_filtering::anisotropic);

  g_elm_handles.reset_bttn_up
      = DTU::load_texture(ids, handles, "resources/ui/character_sheet/bttn_release.png",
                          surge::renderer::texture_filtering::anisotropic);

  g_elm_handles.reset_bttn_down
      = DTU::load_texture(ids, handles, "resources/ui/character_sheet/bttn_press.png",
                          surge::renderer::texture_filtering::anisotropic);

  g_elm_handles.next_page_bttn_up
      = DTU::load_texture(ids, handles, "resources/ui/character_sheet/arrow_bttn_right_release.png",
                          surge::renderer::texture_filtering::anisotropic);

  g_elm_handles.next_page_bttn_down
      = DTU::load_texture(ids, handles, "resources/ui/character_sheet/arrow_bttn_right_press.png",
                          surge::renderer::texture_filtering::anisotropic);

  g_elm_handles.prev_page_bttn_up
      = DTU::load_texture(ids, handles, "resources/ui/character_sheet/arrow_bttn_left_release.png",
                          surge::renderer::texture_filtering::anisotropic);

  g_elm_handles.prev_page_bttn_down
      = DTU::load_texture(ids, handles, "resources/ui/character_sheet/arrow_bttn_left_press.png",
                          surge::renderer::texture_filtering::anisotropic);

  surge::atom::sprite::make_resident(handles);
}

static void sheet_page_0(GLFWwindow *window, DTU::sdl_t &ui_sdl, DTU::tdd_t &tdd, DTU::tgd_t &tgd,
                         surge::i32 &active_widget, surge::i32 &hot_widget, float ww, float wh,
                         const glm::vec2 &mouse_pos, DTU::character::sheet &cs,
                         surge::u8 &page) noexcept {
  using namespace surge;
  using namespace DTU;
  using std::ceil;

  constexpr float sls{55.0f}; // Left shift of the Attributes spinboxes

  // Background
  push_sprite(ui_sdl, g_elm_handles.page_0, make_model(glm::vec3{0.0f}, glm::vec3{ww, wh, 1.0f}),
              1.0f);

  // Empathy
  ui::spinner_box(window, __COUNTER__, active_widget, hot_widget, ui_sdl,
                  glm::vec3{((620.894f - sls) / 1920.0f) * ww, (217.784f / 1080.0f) * wh, 0.1f},
                  glm::vec3{(111.0 / 1920.0f) * ww, (39.520f / 1080.0f) * wh, 1.0f},
                  g_elm_handles.spinner_box_neutral, g_elm_handles.spinner_box_up,
                  g_elm_handles.spinner_box_down, 1.0, tdd, tgd, glm::vec4{1.0f}, mouse_pos,
                  cs.attr_pts, cs.empathy, 0, 5);

  // Linguistics
  ui::spinner_box(window, __COUNTER__, active_widget, hot_widget, ui_sdl,
                  glm::vec3{((620.894f - sls) / 1920.0f) * ww, (307.356f / 1080.0f) * wh, 0.1f},
                  glm::vec3{(111.0 / 1920.0f) * ww, (39.520f / 1080.0f) * wh, 1.0f},
                  g_elm_handles.spinner_box_neutral, g_elm_handles.spinner_box_up,
                  g_elm_handles.spinner_box_down, 1.0, tdd, tgd, glm::vec4{1.0f}, mouse_pos,
                  cs.attr_pts, cs.linguistics, 0, 5);

  // Reasoning
  ui::spinner_box(window, __COUNTER__, active_widget, hot_widget, ui_sdl,
                  glm::vec3{((1106.868f - sls) / 1920.0f) * ww, (217.704f / 1080.0f) * wh, 0.1f},
                  glm::vec3{(111.0 / 1920.0f) * ww, (39.520f / 1080.0f) * wh, 1.0f},
                  g_elm_handles.spinner_box_neutral, g_elm_handles.spinner_box_up,
                  g_elm_handles.spinner_box_down, 1.0, tdd, tgd, glm::vec4{1.0f}, mouse_pos,
                  cs.attr_pts, cs.reasoning, 0, 5);

  // Introspection
  if (ui::spinner_box(
          window, __COUNTER__, active_widget, hot_widget, ui_sdl,
          glm::vec3{((1106.868f - sls) / 1920.0f) * ww, (307.863f / 1080.0f) * wh, 0.1f},
          glm::vec3{(111.0 / 1920.0f) * ww, (39.520f / 1080.0f) * wh, 1.0f},
          g_elm_handles.spinner_box_neutral, g_elm_handles.spinner_box_up,
          g_elm_handles.spinner_box_down, 1.0, tdd, tgd, glm::vec4{1.0f}, mouse_pos, cs.attr_pts,
          cs.introspection, 0, 5)) {
    cs.psyche_pts = 2 * (cs.introspection + 1);
  }

  // Fitness
  if (ui::spinner_box(
          window, __COUNTER__, active_widget, hot_widget, ui_sdl,
          glm::vec3{((1483.158f - sls) / 1920.0f) * ww, (223.304f / 1080.0f) * wh, 0.1f},
          glm::vec3{(111.0 / 1920.0f) * ww, (39.520f / 1080.0f) * wh, 1.0f},
          g_elm_handles.spinner_box_neutral, g_elm_handles.spinner_box_up,
          g_elm_handles.spinner_box_down, 1.0, tdd, tgd, glm::vec4{1.0f}, mouse_pos, cs.attr_pts,
          cs.fitness, 0, 5)) {
    cs.health_pts = 2 * (cs.fitness + 1);
    cs.initiative = static_cast<u8>(
        std::ceil((static_cast<float>(cs.fitness) + static_cast<float>(cs.agility)) / 2.0f));
  }

  // Agility
  if (ui::spinner_box(
          window, __COUNTER__, active_widget, hot_widget, ui_sdl,
          glm::vec3{((1483.158f - sls) / 1920.0f) * ww, (307.356f / 1080.0f) * wh, 0.1f},
          glm::vec3{(111.0 / 1920.0f) * ww, (39.520f / 1080.0f) * wh, 1.0f},
          g_elm_handles.spinner_box_neutral, g_elm_handles.spinner_box_up,
          g_elm_handles.spinner_box_down, 1.0, tdd, tgd, glm::vec4{1.0f}, mouse_pos, cs.attr_pts,
          cs.agility, 0, 5)) {
    cs.action_pts = 2 * (cs.agility + 1);
    cs.initiative = static_cast<u8>(
        std::ceil((static_cast<float>(cs.fitness) + static_cast<float>(cs.agility)) / 2.0f));
  }

  // The scale for the following texts is obteined by computing
  // Resolution of background / resolution of loaded glyphs.
  // This is multiplyied with the current screen sizes over the scree size where the texture was
  // created.

  // HP value
  surge::atom::text::append_text_draw_data(
      tdd, tgd, cs.health_pts,
      glm::vec3{(835.378f / 1920.0f) * ww, (522.421f / 1080.0f) * wh, 0.1f}, glm::vec4{1.0f},
      (96.0f / 300.0f) * glm::vec2{ww / 1920.0f, wh / 1080.0f});

  // AP value
  surge::atom::text::append_text_draw_data(
      tdd, tgd, cs.action_pts,
      glm::vec3{(835.378f / 1920.0f) * ww, (613.806f / 1080.0f) * wh, 0.1f}, glm::vec4{1.0f},
      (96.0f / 300.0f) * glm::vec2{ww / 1920.0f, wh / 1080.0f});

  // PP value
  surge::atom::text::append_text_draw_data(
      tdd, tgd, cs.psyche_pts,
      glm::vec3{(1377.486f / 1920.0f) * ww, (516.901f / 1080.0f) * wh, 0.1f}, glm::vec4{1.0f},
      (96.0f / 300.0f) * glm::vec2{ww / 1920.0f, wh / 1080.0f});

  // IN value
  surge::atom::text::append_text_draw_data(
      tdd, tgd, cs.initiative,
      glm::vec3{(1377.486f / 1920.0f) * ww, (613.779f / 1080.0f) * wh, 0.1f}, glm::vec4{1.0f},
      (96.0f / 300.0f) * glm::vec2{ww / 1920.0f, wh / 1080.0f});

  // Remaining points
  surge::atom::text::append_text_draw_data(
      tdd, tgd, cs.attr_pts, glm::vec3{(246.418f / 1920.0f) * ww, (526.496f / 1080.0f) * wh, 0.1f},
      glm::vec4{1.0f}, (96.0f / 300.0f) * glm::vec2{ww / 1920.0f, wh / 1080.0f});

  // Empathy help text
  ui::text_on_hot(__COUNTER__, 0, hot_widget, tdd, tgd,
                  "Empathy urges you to see the world through\n"
                  "the eyes of others. It is usefull when connecting\n"
                  "to people but also when attempting to mani-\n"
                  "pulate or lie to them.",
                  glm::vec3{(350.404f / 1920.0f) * ww, (780.786f / 1080.0f) * wh, 0.1f},
                  glm::vec4{1.0f}, (96.0f / 300.0f) * glm::vec2{ww / 1920.0f, wh / 1080.0f});

  // Linguistics help text
  ui::text_on_hot(__COUNTER__, 1, hot_widget, tdd, tgd,
                  "Linguistics is your ability to understand and\n"
                  "express yourself in your language. It is usefull\n"
                  "when articulating your thoughts but also when\n"
                  "attempting to fabricate lies or persuade people.",
                  glm::vec3{(350.404f / 1920.0f) * ww, (780.786f / 1080.0f) * wh, 0.1f},
                  glm::vec4{1.0f}, (96.0f / 300.0f) * glm::vec2{ww / 1920.0f, wh / 1080.0f});

  // Reasoning help text
  ui::text_on_hot(__COUNTER__, 2, hot_widget, tdd, tgd,
                  "Reasoning represents your ability to emply lo-\n"
                  "gical and abstract thinking to solve problems.\n"
                  "This is usefull when making logical decisions\n"
                  "and deductions.",
                  glm::vec3{(350.404f / 1920.0f) * ww, (780.786f / 1080.0f) * wh, 0.1f},
                  glm::vec4{1.0f}, (96.0f / 300.0f) * glm::vec2{ww / 1920.0f, wh / 1080.0f});

  // Introspection help text
  ui::text_on_hot(__COUNTER__, 3, hot_widget, tdd, tgd,
                  "Introspection is the ability to understand and\n"
                  "interpret your own feelings towards other people\n"
                  "and situations. This understanding helps you\n"
                  "when attempting to fulfill your desires.",
                  glm::vec3{(350.404f / 1920.0f) * ww, (780.786f / 1080.0f) * wh, 0.1f},
                  glm::vec4{1.0f}, (96.0f / 300.0f) * glm::vec2{ww / 1920.0f, wh / 1080.0f});

  // Fitness help text
  ui::text_on_hot(__COUNTER__, 4, hot_widget, tdd, tgd,
                  "Fitness represents your overall physical health\n"
                  "and bodily shape. It is usefull when attempting\n"
                  "to perform physical feats of strength. It also\n"
                  "determines how much helath you have.",
                  glm::vec3{(350.404f / 1920.0f) * ww, (780.786f / 1080.0f) * wh, 0.1f},
                  glm::vec4{1.0f}, (96.0f / 300.0f) * glm::vec2{ww / 1920.0f, wh / 1080.0f});

  // Agility help text
  ui::text_on_hot(__COUNTER__, 5, hot_widget, tdd, tgd,
                  "Agility represents your overall dexterity and the\n"
                  "ability over your fine motor skills. This is usefull\n"
                  "when attempting to perform feats that require\n"
                  "fine or well coordinated movements of the body.",
                  glm::vec3{(350.404f / 1920.0f) * ww, (780.786f / 1080.0f) * wh, 0.1f},
                  glm::vec4{1.0f}, (96.0f / 300.0f) * glm::vec2{ww / 1920.0f, wh / 1080.0f});

  // Reset Bttn
  if (ui::button(window, __COUNTER__, active_widget, hot_widget, ui_sdl,
                 glm::vec3{(99.298f / 1920.0f) * ww, (551.294f / 1080.0f) * wh, 0.1f},
                 glm::vec3{(175.575f / 1920.0f) * ww, (52.680f / 1080.0f) * wh, 1.0f},
                 g_elm_handles.reset_bttn_up, g_elm_handles.reset_bttn_down, 1.0f, tdd, tgd,
                 glm::vec4{1.0f}, mouse_pos, "Reset")) {
    cs = character::sheet{};
  }

  // Page change bttn
  if (ui::button(window, __COUNTER__, active_widget, hot_widget, ui_sdl,
                 glm::vec3{(1660.000f / 1920.0f) * ww, (800.0f / 1080.0f) * wh, 0.1f},
                 glm::vec3{(120.0f / 1920.0f) * ww, (102.740f / 1080.0f) * wh, 1.0f},
                 g_elm_handles.next_page_bttn_up, g_elm_handles.next_page_bttn_down, 1.0f,
                 mouse_pos)) {
    if (cs.attr_pts == 0) {
      page = 1;
    }
  }

  // Text if page change is not allowed
  if (hot_widget == 13 && cs.attr_pts != 0) {
    atom::text::append_text_draw_data(
        tdd, tgd, "Spend all skill points to proceed.",
        glm::vec3{(350.404f / 1920.0f) * ww, (780.786f / 1080.0f) * wh, 0.1f}, glm::vec4{1.0f},
        (96.0f / 300.0f) * glm::vec2{ww / 1920.0f, wh / 1080.0f});
  }
}

static void sheet_page_1(GLFWwindow *window, DTU::sdl_t &ui_sdl, DTU::tdd_t /*&tdd*/,
                         DTU::tgd_t /*&tgd*/, surge::i32 &active_widget, surge::i32 &hot_widget,
                         float ww, float wh, const glm::vec2 &mouse_pos,
                         DTU::character::sheet /*&cs*/, surge::u8 &page) noexcept {
  using namespace surge;
  using namespace DTU;

  // Background
  push_sprite(ui_sdl, g_elm_handles.page_1, make_model(glm::vec3{0.0f}, glm::vec3{ww, wh, 1.0f}),
              1.0f);

  // Page change bttn
  if (ui::button(window, __COUNTER__, active_widget, hot_widget, ui_sdl,
                 glm::vec3{(140.000f / 1920.0f) * ww, (800.0f / 1080.0f) * wh, 0.1f},
                 glm::vec3{(120.0f / 1920.0f) * ww, (102.740f / 1080.0f) * wh, 1.0f},
                 g_elm_handles.prev_page_bttn_up, g_elm_handles.prev_page_bttn_down, 1.0f,
                 mouse_pos)) {
    page = 0;
  }
}

void DTU::ui::character_sheet::update(GLFWwindow *window, sdl_t &ui_sdl, tdd_t &tdd,
                                      tgd_t &tgd) noexcept {
  using std::ceil;
  using namespace surge;

  static character::sheet cs{};
  static i32 active_widget{-1};
  static i32 hot_widget{-1};
  static u8 page{0};

  const auto [ww, wh] = window::get_dims(window);
  const auto mouse_pos{window::get_cursor_pos(window)};

  switch (page) {
  case 0:
    sheet_page_0(window, ui_sdl, tdd, tgd, active_widget, hot_widget, ww, wh, mouse_pos, cs, page);
    break;

  case 1:
    sheet_page_1(window, ui_sdl, tdd, tgd, active_widget, hot_widget, ww, wh, mouse_pos, cs, page);
    break;

  default:
    break;
  }
}