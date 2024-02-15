#include "new_game/new_game.hpp"

#include "player/logging.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <gsl/gsl-lite.hpp>

static constexpr const surge::usize ui_elms{8};
static std::array<GLuint, ui_elms> g_ui_elms_ids{};
static std::array<GLuint64, ui_elms> g_ui_elms_handles{};

// UI positional data

namespace help {
constexpr const glm::vec4 text_area_rect{542.092f, 569.588f, 451.815f, 176.365f};
} // namespace help

namespace empathy {

constexpr const glm::vec4 group_rect{29.466f, 152.916f, 199.319f, 36.322f};
// constexpr const glm::vec4 rank_rect{172.285f, 152.916f, 36.321f, 36.321f};
constexpr const glm::vec4 button_rect{212.642f, 152.916f, 16.143f, 36.322f};

constexpr const glm::vec4 up_rect{button_rect[0], button_rect[1], button_rect[2],
                                  button_rect[3] / 2.0f};
constexpr const glm::vec4 down_rect{button_rect[0], button_rect[1] + button_rect[3] / 2.0f,
                                    button_rect[2], button_rect[3] / 2.0f};
} // namespace empathy

namespace introspection {

constexpr const glm::vec4 group_rect{233.784f, 152.916f, 249.272f, 36.321f};

} // namespace introspection

static auto point_in_rect(const glm::vec2 &point, const glm::vec4 &rect) noexcept -> bool {
  const auto x0{rect[0]};
  const auto xf{rect[0] + rect[2]};

  const auto y0{rect[1]};
  const auto yf{rect[1] + rect[3]};

  const auto xbound{x0 < point[0] && point[0] < xf};
  const auto ybound{y0 < point[1] && point[1] < yf};

  return xbound && ybound;
}

static void help_text(GLFWwindow *window, surge::atom::text::text_draw_data &tdd,
                      surge::atom::text::glyph_data &tgd) noexcept {
  const auto cursor_pos{surge::window::get_cursor_pos(window)};

  if (point_in_rect(cursor_pos, empathy::group_rect)) {
    surge::atom::text::overwrite_text_draw_data(
        tdd, tgd,
        "Empathy represents your abil-\n"
        "ity to detect, understand and\n"
        "empathize with the feelings of\n"
        "others.",
        glm::vec3{help::text_area_rect[0], help::text_area_rect[1] + 32.0f, 0.1f});

  } else if (point_in_rect(cursor_pos, introspection::group_rect)) {
    surge::atom::text::overwrite_text_draw_data(
        tdd, tgd,
        "Introspection represents your\n"
        "ability to understand yourself,\n"
        "your feelings, desires, fears\n"
        "and overall mental state.",
        glm::vec3{help::text_area_rect[0], help::text_area_rect[1] + 32.0f, 0.1f});

  } else {
    tdd.texture_handles.clear();
    tdd.glyph_models.clear();
  }
}

static void load_gui_elms() noexcept {
  using namespace surge;

  log_info("Loading UI elements");

  const std::array<const char *, ui_elms> img_names{
      "resources/new_game/character_sheet.png",
      "resources/new_game/drop.png",
      "resources/new_game/d4.png",
      "resources/new_game/d6.png",
      "resources/new_game/d8.png",
      "resources/new_game/d10.png",
      "resources/new_game/d12.png",
      "resources/new_game/d20.png",
  };

  for (surge::usize i = 0; const auto &image_name : img_names) {
    auto img{files::load_image(image_name)};

    if (img) {
      const auto texture_data{
          atom::sprite::create_texture(*img, renderer::texture_filtering::nearest)};

      if (texture_data) {
        g_ui_elms_ids[i] = std::get<0>(*texture_data);
        g_ui_elms_handles[i] = std::get<1>(*texture_data);
        surge::atom::sprite::make_resident(std::get<1>(*texture_data));
      } else {
        g_ui_elms_handles[i] = 0;
        g_ui_elms_ids[i] = 0;
      }

      files::free_image(*img);
    }
    i++;
  }
}

auto DTU::state::new_game::load(surge::deque<surge::u32> &, surge::atom::sprite::data_list &dl,
                                float ww, float wh) noexcept -> int {
  using namespace surge;

  log_info("Loading new_game state");

  // Character Sheet
  load_gui_elms();

  dl.texture_ids.push_back(g_ui_elms_ids[0]);
  dl.texture_handles.push_back(g_ui_elms_handles[0]);
  dl.alphas.push_back(1.0);
  dl.models.push_back(glm::scale(glm::translate(glm::mat4{1.0f}, glm::vec3{0.0f, 0.0f, 0.0f}),
                                 glm::vec3{ww, wh, 1.0}));

  return 0;
}

void DTU::state::new_game::unload(surge::deque<surge::u32> &cmdq,
                                  surge::atom::sprite::data_list &dl) noexcept {
  using namespace surge;

  log_info("Unloading main_menu state");

  for (const auto &handle : g_ui_elms_handles) {
    atom::sprite::make_non_resident(handle);
  }

  for (const auto &id : g_ui_elms_ids) {
    atom::sprite::destroy_texture(id);
  }

  dl.texture_handles.clear();
  dl.texture_ids.clear();
  dl.alphas.clear();
  dl.models.clear();

  cmdq.clear();

  return;
}

void DTU::state::new_game::update(GLFWwindow *window, surge::deque<surge::u32> &cmdq,
                                  surge::atom::sprite::data_list &,
                                  surge::atom::text::text_draw_data &tdd,
                                  surge::atom::text::glyph_data &tgd, double) noexcept {

  help_text(window, tdd, tgd);

  switch (cmdq.size() == 0 ? commands::idle : cmdq.front()) {
  default:
    break;
  }
}

void DTU::state::new_game::mouse_click(GLFWwindow *window, int button, int action, int) noexcept {
  const bool left_click{button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS};
  const auto cursor_pos{surge::window::get_cursor_pos(window)};

  if (left_click && point_in_rect(cursor_pos, empathy::up_rect)) {
    log_info("Empathy up");
  }

  if (left_click && point_in_rect(cursor_pos, empathy::down_rect)) {
    log_info("Empathy down");
  }
}

void DTU::state::new_game::mouse_scroll(GLFWwindow *window, double, double yoffset) noexcept {
  using namespace surge;
  const auto cursor_pos{surge::window::get_cursor_pos(window)};

  if (yoffset > 0 && point_in_rect(cursor_pos, empathy::group_rect)) {
    log_info("Empathy up");
  }

  if (yoffset < 0 && point_in_rect(cursor_pos, empathy::group_rect)) {
    log_info("Empathy down");
  }
}
