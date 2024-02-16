#include "new_game/new_game.hpp"

#include "new_game/ui_positional_data.hpp"
#include "player/logging.hpp"

#include <cstdio>
#include <glm/gtc/matrix_transform.hpp>
#include <gsl/gsl-lite.hpp>

static constexpr const surge::usize ui_elms{8};
static std::array<GLuint, ui_elms> g_ui_elms_ids{};
static std::array<GLuint64, ui_elms> g_ui_elms_handles{};

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

static void ui_text(const DTU::state::new_game::ui_state &state,
                    surge::atom::text::text_draw_data &ptb, surge::atom::text::glyph_data &tgd) {
  using std::snprintf;

  constexpr const surge::usize buffer_size{3};
  std::array<char, buffer_size> buffer{0, 0, 0};

  snprintf(buffer.data(), buffer_size, "%d", state.empathy_rank);
  surge::atom::text::overwrite_text_draw_data(
      ptb, tgd, buffer.data(),
      glm::vec3{empathy::rank_baseline[0], empathy::rank_baseline[1], 0.1f});

  snprintf(buffer.data(), buffer_size, "%d", state.remaining_points);
  surge::atom::text::append_text_draw_data(
      ptb, tgd, buffer.data(),
      glm::vec3{remaining_points::value_baseline[0], remaining_points::value_baseline[1], 0.1f},
      glm::vec4{1.0f, 1.0f, 1.0f, 1.0f});
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
                                  surge::atom::text::text_draw_data &ptb,
                                  surge::atom::text::text_draw_data &etb,
                                  surge::atom::text::glyph_data &tgd, double) noexcept {
  static ui_state uistate{};

  switch (cmdq.size() == 0 ? commands::idle : cmdq.front()) {

  case commands::empathy_up:
    if (uistate.empathy_rank + 1 <= 5 && uistate.remaining_points >= 1) {
      uistate.empathy_rank += 1;
      uistate.remaining_points -= 1;
    }
    cmdq.pop_front();
    break;

  case commands::empathy_down:
    if (uistate.empathy_rank - 1 >= 0) {
      uistate.empathy_rank -= 1;
      uistate.remaining_points += 1;
    }
    cmdq.pop_front();
    break;

  default:
    break;
  }

  ui_text(uistate, ptb, tgd);
  help_text(window, etb, tgd);
}

void DTU::state::new_game::mouse_click(surge::deque<surge::u32> &cmdq, GLFWwindow *window,
                                       int button, int action, int) noexcept {
  const bool left_click{button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS};
  const auto cursor_pos{surge::window::get_cursor_pos(window)};

  if (left_click && point_in_rect(cursor_pos, empathy::up_rect)) {
    cmdq.push_back(commands::empathy_up);
  }

  if (left_click && point_in_rect(cursor_pos, empathy::down_rect)) {
    cmdq.push_back(commands::empathy_down);
  }
}

void DTU::state::new_game::mouse_scroll(surge::deque<surge::u32> &cmdq, GLFWwindow *window, double,
                                        double yoffset) noexcept {
  using namespace surge;
  const auto cursor_pos{surge::window::get_cursor_pos(window)};

  if (yoffset > 0 && point_in_rect(cursor_pos, empathy::group_rect)) {
    cmdq.push_back(commands::empathy_up);
  }

  if (yoffset < 0 && point_in_rect(cursor_pos, empathy::group_rect)) {
    cmdq.push_back(commands::empathy_down);
  }
}
