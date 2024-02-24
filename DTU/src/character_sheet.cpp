
// clang-format off
#include "DTU.hpp"
#include "character_sheet.hpp"
#include "character.hpp"
#include "ui.hpp"

#include "player/logging.hpp"
// clang-format on

struct element_handles {
  GLuint64 spinner_box{0};
};

static element_handles g_elm_handles{}; // NOLINT

void DTU::ui::character_sheet::load(vec_glui &ids, vec_glui64 &handles) noexcept {
  g_elm_handles.spinner_box
      = DTU::load_texture(ids, handles, "resources/ui/character_sheet/spinner_box.png",
                          surge::renderer::texture_filtering::linear);

  surge::atom::sprite::make_resident(handles);
}

void DTU::ui::character_sheet::update(GLFWwindow *window, const sbd_t &ui_sbd,
                                      sdl_t &ui_sdl) noexcept {
  using std::snprintf;
  using namespace surge;

  DTU::clear_sprites(ui_sdl);
  // DTU::clear_text(tdd); TODO when ther is text

  static character::sheet cs{};

  const auto cursor_pos{window::get_cursor_pos(window)};

  std::array<char, 3> buffer{0, 0, 0};

  snprintf(buffer.data(), 3, "%u", cs.empathy);
  bool up{false};
  if (ui::spinner_box(cursor_pos, glm::vec3{1024.0f / 2.0f, 768.0f / 2.0f, 0.0f},
                      g_elm_handles.spinner_box, ui_sdl, "text", up)) {
    // TODO
    surge::atom::sprite::send_buffers(ui_sbd, ui_sdl);
  }

  // TODO
}

void DTU::ui::character_sheet::mouse_left_click(cmdq_t &, const glm::vec2 &) noexcept {
  // TODO
}

void DTU::ui::character_sheet::mouse_scroll_up(cmdq_t &, const glm::vec2 &) noexcept {
  // TODO
}

void DTU::ui::character_sheet::mouse_scroll_down(cmdq_t &, const glm::vec2 &) noexcept {
  // TODO
}