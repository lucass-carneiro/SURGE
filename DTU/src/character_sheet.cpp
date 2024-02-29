
// clang-format off
#include "DTU.hpp"
#include "character_sheet.hpp"
#include "character.hpp"
#include "ui.hpp"

#include "player/logging.hpp"
// clang-format on

struct element_handles {
  GLuint64 page_0{0};
  GLuint64 spinner_box_neutral{0};
  GLuint64 spinner_box_up{0};
  GLuint64 spinner_box_down{0};
};

static element_handles g_elm_handles{}; // NOLINT

void DTU::ui::character_sheet::load(vec_glui &ids, vec_glui64 &handles) noexcept {
  g_elm_handles.page_0
      = DTU::load_texture(ids, handles, "resources/ui/character_sheet/character_sheet_pg_0.png",
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

  surge::atom::sprite::make_resident(handles);
}

void DTU::ui::character_sheet::update(GLFWwindow *window, sdl_t &ui_sdl, tdd_t &tdd,
                                      tgd_t &tgd) noexcept {
  using std::snprintf;
  using namespace surge;

  static character::sheet cs{};
  static i32 active_widget{-1};
  static i32 hot_widget{-1};

  const auto [ww, wh] = window::get_dims(window);
  const auto mouse_pos{window::get_cursor_pos(window)};

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
  ui::spinner_box(window, __COUNTER__, active_widget, hot_widget, ui_sdl,
                  glm::vec3{((1106.868f - sls) / 1920.0f) * ww, (307.863f / 1080.0f) * wh, 0.1f},
                  glm::vec3{(111.0 / 1920.0f) * ww, (39.520f / 1080.0f) * wh, 1.0f},
                  g_elm_handles.spinner_box_neutral, g_elm_handles.spinner_box_up,
                  g_elm_handles.spinner_box_down, 1.0, tdd, tgd, glm::vec4{1.0f}, mouse_pos,
                  cs.attr_pts, cs.introspection, 0, 5);

  // Fitness
  ui::spinner_box(window, __COUNTER__, active_widget, hot_widget, ui_sdl,
                  glm::vec3{((1483.158f - sls) / 1920.0f) * ww, (223.304f / 1080.0f) * wh, 0.1f},
                  glm::vec3{(111.0 / 1920.0f) * ww, (39.520f / 1080.0f) * wh, 1.0f},
                  g_elm_handles.spinner_box_neutral, g_elm_handles.spinner_box_up,
                  g_elm_handles.spinner_box_down, 1.0, tdd, tgd, glm::vec4{1.0f}, mouse_pos,
                  cs.attr_pts, cs.fitness, 0, 5);

  // Agility
  ui::spinner_box(window, __COUNTER__, active_widget, hot_widget, ui_sdl,
                  glm::vec3{((1483.158f - sls) / 1920.0f) * ww, (307.356f / 1080.0f) * wh, 0.1f},
                  glm::vec3{(111.0 / 1920.0f) * ww, (39.520f / 1080.0f) * wh, 1.0f},
                  g_elm_handles.spinner_box_neutral, g_elm_handles.spinner_box_up,
                  g_elm_handles.spinner_box_down, 1.0, tdd, tgd, glm::vec4{1.0f}, mouse_pos,
                  cs.attr_pts, cs.agility, 0, 5);

  // The scale for the following texts is obteined by computing
  // Resolution of background / resolution of loaded glyphs.
  // This is multiplyied with the current screen sizes over the scree size where the texture was
  // created.

  // "The quick brown fox jumps over the lazy dog. Th\n",

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
}