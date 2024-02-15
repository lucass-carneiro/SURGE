#include "new_game/new_game.hpp"

#include "player/logging.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <gsl/gsl-lite.hpp>

static constexpr const surge::usize ui_elms{8};
static std::array<GLuint, ui_elms> g_ui_elms_ids{};
static std::array<GLuint64, ui_elms> g_ui_elms_handles{};

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

void DTU::state::new_game::update(surge::deque<surge::u32> &cmdq, surge::atom::sprite::data_list &,
                                  double) noexcept {

  switch (cmdq.size() == 0 ? commands::idle : cmdq.front()) {
  default:
    break;
  }
}

constexpr const glm::vec2 attr_bttn_dims{16.143f, 36.322f};

constexpr const glm::vec2 empathy_pos{212.642f, 152.916f};
constexpr const glm::vec4 empathy_up_rect{empathy_pos[0], empathy_pos[1], attr_bttn_dims[0],
                                          attr_bttn_dims[1] / 2.0f};
constexpr const glm::vec4 empathy_down_rect{empathy_pos[0],
                                            empathy_pos[1] + attr_bttn_dims[1] / 2.0f,
                                            attr_bttn_dims[0], attr_bttn_dims[1] / 2.0f};

static auto point_in_rect(const glm::vec2 &point, const glm::vec4 &rect) noexcept -> bool {
  const auto x0{rect[0]};
  const auto xf{rect[0] + rect[2]};

  const auto y0{rect[1]};
  const auto yf{rect[1] + rect[3]};

  const auto xbound{x0 < point[0] && point[0] < xf};
  const auto ybound{y0 < point[1] && point[1] < yf};

  return xbound && ybound;
}

void DTU::state::new_game::mouse_click(GLFWwindow *window, int button, int action, int) noexcept {
  const bool left_click{button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS};
  const auto cursor_pos{surge::window::get_cursor_pos(window)};

  if (left_click && point_in_rect(cursor_pos, empathy_up_rect)) {
    log_info("Empathy up");
  }

  if (left_click && point_in_rect(cursor_pos, empathy_down_rect)) {
    log_info("Empathy down");
  }
}

void DTU::state::new_game::mouse_scroll(GLFWwindow *window, double, double yoffset) noexcept {
  using namespace surge;
  const auto cursor_pos{surge::window::get_cursor_pos(window)};

  if (yoffset > 0 && point_in_rect(cursor_pos, empathy_up_rect)) {
    log_info("Empathy up");
  }

  if (yoffset < 0 && point_in_rect(cursor_pos, empathy_up_rect)) {
    log_info("Empathy down");
  }
}
