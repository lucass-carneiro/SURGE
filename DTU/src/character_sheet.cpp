
// clang-format off
#include "DTU.hpp"
#include "character_sheet.hpp"
#include "character.hpp"
#include "ui.hpp"

#include "player/logging.hpp"
// clang-format on

struct element_handles {
  GLuint64 spinner_box_neutral{0};
  GLuint64 spinner_box_up{0};
  GLuint64 spinner_box_down{0};
};

static element_handles g_elm_handles{}; // NOLINT

void DTU::ui::character_sheet::load(vec_glui &ids, vec_glui64 &handles) noexcept {
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

  const auto mouse_pos{window::get_cursor_pos(window)};

  if (ui::spinner_box(window, __COUNTER__, active_widget, hot_widget, ui_sdl,
                      glm::vec3{1024.0f / 2.0f, 768.0f / 2.0f, 0.0f},
                      glm::vec3{347.0f, 116.0f, 1.0f}, g_elm_handles.spinner_box_neutral,
                      g_elm_handles.spinner_box_up, g_elm_handles.spinner_box_down, 1.0, tdd, tgd,
                      glm::vec4{1.0f}, mouse_pos, cs.empathy, 0, 5)) {
    // TODO
  }
}