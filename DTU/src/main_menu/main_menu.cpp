#include "main_menu/main_menu.hpp"

#include "player/files.hpp"
#include "player/logging.hpp"
#include "player/sprite.hpp"
#include "player/window.hpp"
#include "states.hpp"

#include <array>
#include <glm/gtc/matrix_transform.hpp>
#include <gsl/gsl-lite.hpp>
#include <optional>
#include <span>

// Parallax background
static constexpr const GLsizei background_layer_count{5};

// NOLINTNEXTLINE
// static DTU::state::main_menu::ui_states g_ui_state{DTU::state::main_menu::ui_states::hidden};

// clang-format off
// Obtained from the cubic polynomial (4 + x + x^2) / 400
static constexpr const std::array<float, background_layer_count> background_drift_speeds{
    1.0f / 100.0f,
    3.0f / 200.0f,
    1.0f / 40.0f,
    1.0f / 25.0f,
    3.0f / 50.0f
};
// clang-format on

static void load_background_images(surge::atom::sprite::data_list &dl) noexcept {
  using namespace surge;

  log_info("Loading background images");

  const std::array<const char *, background_layer_count> background_image_names{
      "resources/main_menu/1.png", "resources/main_menu/2.png", "resources/main_menu/3.png",
      "resources/main_menu/4.png", "resources/main_menu/5.png"};

  // Load images
  for (const auto &image_name : background_image_names) {
    auto img{files::load_image(image_name)};

    if (img) {
      const auto texture_data{
          atom::sprite::create_texture(*img, renderer::texture_filtering::nearest)};

      if (texture_data) {
        dl.texture_ids.push_back(std::get<0>(*texture_data));
        dl.texture_handles.push_back(std::get<1>(*texture_data));
        dl.alphas.push_back(1.0f);
      } else {
        dl.texture_ids.push_back(0);
        dl.texture_handles.push_back(0);
        dl.alphas.push_back(0.0f);
      }

      files::free_image(*img);
    }
  }
}

static void load_background_quads(surge::atom::sprite::data_list &dl, float ww, float wh) noexcept {
  using namespace surge;

  for (usize i = 0; i < background_layer_count; i++) {
    dl.models.push_back(
        glm::scale(glm::translate(glm::mat4{1.0f}, glm::vec3{0.0f, 0.0f, 0.0f + i / 10.0f}),
                   glm::vec3{ww * 2.0f, wh, 1.0}));
  }
}

static void update_background_quads(surge::atom::sprite::data_list &dl, double dt) noexcept {
  using namespace surge;

  static std::array<float, background_layer_count> relative_positions{0};
  const auto dtf{gsl::narrow_cast<float>(dt)};

  for (usize i = 0; auto &model : std::span(dl.models).first(background_layer_count)) {
    const auto drift_speed{background_drift_speeds[i]};

    if (relative_positions[i] < 0.5f) {
      model = glm::translate(model, glm::vec3{-drift_speed * dtf, 0.0f, 0.0f});
      relative_positions[i] += drift_speed * dtf;
    } else {
      model = glm::translate(model, glm::vec3{0.5f, 0.0f, 0.0f});
      relative_positions[i] = 0;
    }
    i++;
  }
}

static void load_title_image(surge::atom::sprite::data_list &dl) noexcept {
  using namespace surge;

  log_info("Loading title images");

  auto title_image{files::load_image("resources/main_menu/title.png")};

  if (title_image) {
    const auto handle{atom::sprite::create_texture(*title_image)};
    files::free_image(*title_image);

    if (handle) {
      dl.texture_ids.push_back(std::get<0>(*handle));
      dl.texture_handles.push_back(std::get<1>(*handle));
    } else {
      dl.texture_ids.push_back(0);
      dl.texture_handles.push_back(0);
    }

    dl.alphas.push_back(0.0f);
  }
}

static void load_title_quad(surge::atom::sprite::data_list &dl, float ww, float wh) noexcept {
  dl.models.push_back(glm::scale(
      glm::translate(glm::mat4{1.0f}, glm::vec3{(ww - 608.0f) / 2.0f, (wh - 174.0f) / 2.0f, 0.5f}),
      glm::vec3{608.0f, 174.0f, 1.0}));
}

static void load_options_images(surge::atom::sprite::data_list &dl) noexcept {
  using namespace surge;

  log_info("Loading options images");

  const std::array<const char *, 6> opt_img_names{
      "resources/main_menu/menu_new_game.png", "resources/main_menu/menu_load_game.png",
      "resources/main_menu/menu_options.png",  "resources/main_menu/menu_credits.png",
      "resources/main_menu/menu_exit.png",     "resources/main_menu/menu_border.png"};

  // Load images
  for (const auto &image_name : opt_img_names) {
    auto img{files::load_image(image_name)};

    if (img) {
      const auto texture_data{
          atom::sprite::create_texture(*img, renderer::texture_filtering::nearest)};

      if (texture_data) {
        dl.texture_ids.push_back(std::get<0>(*texture_data));
        dl.texture_handles.push_back(std::get<1>(*texture_data));
      } else {
        dl.texture_ids.push_back(0);
        dl.texture_handles.push_back(0);
      }

      files::free_image(*img);
      dl.alphas.push_back(0.0f);
    }
  }
}

static void load_options_quads(surge::atom::sprite::data_list &dl, float ww, float wh) noexcept {

  dl.models.push_back(glm::scale(
      glm::translate(glm::mat4{1.0f},
                     glm::vec3{(ww - 448.0f) / 2.0f, (wh - 133.0f) / 2.0f + 133.0f + 50.0f, 0.6f}),
      glm::vec3{448.0f, 133.0f, 1.0}));

  dl.models.push_back(glm::scale(
      glm::translate(glm::mat4{1.0f}, glm::vec3{(ww - 448.0f) / 2.0f + 448.0f,
                                                (wh - 133.0f) / 2.0f + 133.0f + 50.0f, 0.6f}),
      glm::vec3{448.0f, 133.0f, 1.0}));

  dl.models.push_back(glm::scale(
      glm::translate(glm::mat4{1.0f}, glm::vec3{(ww - 448.0f) / 2.0f + 448.0f,
                                                (wh - 133.0f) / 2.0f + 133.0f + 50.0f, 0.6f}),
      glm::vec3{448.0f, 133.0f, 1.0}));

  dl.models.push_back(glm::scale(
      glm::translate(glm::mat4{1.0f}, glm::vec3{(ww - 448.0f) / 2.0f + 448.0f,
                                                (wh - 133.0f) / 2.0f + 133.0f + 50.0f, 0.6f}),
      glm::vec3{448.0f, 133.0f, 1.0}));

  dl.models.push_back(glm::scale(
      glm::translate(glm::mat4{1.0f}, glm::vec3{(ww - 448.0f) / 2.0f + 448.0f,
                                                (wh - 133.0f) / 2.0f + 133.0f + 50.0f, 0.6f}),
      glm::vec3{448.0f, 133.0f, 1.0}));

  dl.models.push_back(glm::scale(
      glm::translate(glm::mat4{1.0f},
                     glm::vec3{(ww - 448.0f) / 2.0f, (wh - 133.0f) / 2.0f + 133.0f + 50.0f, 0.7f}),
      glm::vec3{448.0f, 133.0f, 1.0}));
}

struct entity_indices {
  surge::usize title_idx;
  surge::usize new_game_idx;
  surge::usize load_game_idx;
  surge::usize options_index;
  surge::usize credits_idx;
  surge::usize exit_idx;
  surge::usize border_idx;
};

static auto do_shift_opt_left(surge::usize &current_opt_idx, surge::atom::sprite::data_list &dl,
                              float dt) noexcept -> bool {

  // Do not shift past the exit option
  const auto next_opt_index{current_opt_idx + 1};

  if (next_opt_index > 10) {
    return true;
  }

  const float speed{1.5f * dt};

  bool shift_opts{false};
  bool fade_current_opt{false};
  bool unfade_next_opt{false};

  static const auto opt_initial_x_pos{dl.models[current_opt_idx][3][0]};

  if (dl.models[current_opt_idx][3][0] > (opt_initial_x_pos - 448.0f)) {
    dl.models[current_opt_idx]
        = glm::translate(dl.models[current_opt_idx], glm::vec3{-speed, 0.0f, 0.0f});
    dl.models[next_opt_index]
        = glm::translate(dl.models[next_opt_index], glm::vec3{-speed, 0.0f, 0.0f});
  } else {
    dl.models[current_opt_idx][3][0] = opt_initial_x_pos - 448.0f;
    dl.models[next_opt_index][3][0] = opt_initial_x_pos;
    shift_opts = true;
  }

  if (dl.alphas[current_opt_idx] > 0.0f) {
    dl.alphas[current_opt_idx] -= speed;
  } else {
    dl.alphas[current_opt_idx] = 0.0f;
    fade_current_opt = true;
  }

  if (dl.alphas[next_opt_index] < 1.0f) {
    dl.alphas[next_opt_index] += speed;
  } else {
    dl.alphas[next_opt_index] = 1.0f;
    unfade_next_opt = true;
  }

  if (shift_opts && fade_current_opt && unfade_next_opt) {
    current_opt_idx = next_opt_index;
    return true;
  } else {
    return false;
  }
}

static auto do_shift_opt_right(surge::usize &current_opt_idx, surge::atom::sprite::data_list &dl,
                               float dt) noexcept -> bool {

  // Do not shift past the exit new game
  const auto next_opt_index{current_opt_idx - 1};

  if (next_opt_index < 6) {
    return true;
  }

  const float speed{1.5f * dt};

  bool shift_opts{false};
  bool fade_current_opt{false};
  bool unfade_next_opt{false};

  static const auto opt_initial_x_pos{dl.models[current_opt_idx][3][0]};

  if (dl.models[current_opt_idx][3][0] < (opt_initial_x_pos + 448.0f)) {
    dl.models[current_opt_idx]
        = glm::translate(dl.models[current_opt_idx], glm::vec3{speed, 0.0f, 0.0f});
    dl.models[next_opt_index]
        = glm::translate(dl.models[next_opt_index], glm::vec3{speed, 0.0f, 0.0f});
  } else {
    dl.models[current_opt_idx][3][0] = opt_initial_x_pos + 448.0f;
    dl.models[next_opt_index][3][0] = opt_initial_x_pos;
    shift_opts = true;
  }

  if (dl.alphas[current_opt_idx] > 0.0f) {
    dl.alphas[current_opt_idx] -= speed;
  } else {
    dl.alphas[current_opt_idx] = 0.0f;
    fade_current_opt = true;
  }

  if (dl.alphas[next_opt_index] < 1.0f) {
    dl.alphas[next_opt_index] += speed;
  } else {
    dl.alphas[next_opt_index] = 1.0f;
    unfade_next_opt = true;
  }

  if (shift_opts && fade_current_opt && unfade_next_opt) {
    current_opt_idx = next_opt_index;
    return true;
  } else {
    return false;
  }
}

static void do_enter_option(surge::usize current_opt_idx) noexcept {
  switch (current_opt_idx) {
  case 10:
    DTU::state_machine::push_state(DTU::state_machine::states::exit_game);
    break;

  case 6:
    DTU::state_machine::push_state(DTU::state_machine::states::new_game);
    break;

  default:
    break;
  }
}

auto DTU::state::main_menu::load(surge::deque<surge::u32> &cmdq, surge::atom::sprite::data_list &dl,
                                 float ww, float wh) noexcept -> int {
  using namespace surge;

  log_info("Loading main_menu state");

  // Background
  load_background_images(dl);
  load_background_quads(dl, ww, wh);

  // Title
  load_title_image(dl);
  load_title_quad(dl, ww, wh);

  // Options
  load_options_images(dl);
  load_options_quads(dl, ww, wh);

  // Make all loaded resident
  atom::sprite::make_resident(dl.texture_handles);

  // First command
  cmdq.push_back(commands::show_title);

  return 0;
}

void DTU::state::main_menu::unload(surge::deque<surge::u32> &cmdq,
                                   surge::atom::sprite::data_list &dl) noexcept {
  using namespace surge;

  log_info("Unloading main_menu state");

  atom::sprite::make_non_resident(dl.texture_handles);
  atom::sprite::destroy_texture(dl.texture_ids);
  dl.texture_handles.clear();
  dl.texture_ids.clear();
  dl.alphas.clear();
  dl.models.clear();

  cmdq.clear();
}

void DTU::state::main_menu::update(surge::deque<surge::u32> &cmdq,
                                   surge::atom::sprite::data_list &dl, double dt) noexcept {

  update_background_quads(dl, dt);

  const entity_indices ei{dl.models.size() - 7, dl.models.size() - 6, dl.models.size() - 5,
                          dl.models.size() - 4, dl.models.size() - 3, dl.models.size() - 2,
                          dl.models.size() - 1};

  static auto current_opt_idx{ei.new_game_idx};

  switch (cmdq.size() == 0 ? commands::idle : cmdq.front()) {

  case commands::show_title:
    if (dl.alphas[ei.title_idx] < 1.0f) {
      dl.alphas[ei.title_idx] += 1.0f * gsl::narrow_cast<float>(dt);
    } else {
      dl.alphas[ei.title_idx] = 1.0f;
      cmdq.pop_front();
    }
    break;

  case commands::show_menu:
    if (dl.alphas[ei.border_idx] < 1.0f) {
      dl.alphas[ei.border_idx] += 1.0f * gsl::narrow_cast<float>(dt);
      dl.alphas[ei.new_game_idx] += 1.0f * gsl::narrow_cast<float>(dt);
    } else {
      dl.alphas[ei.border_idx] = 1.0f;
      dl.alphas[ei.new_game_idx] = 1.0f;
      cmdq.pop_front();
    }
    break;

  case commands::shift_opt_left:
    if (do_shift_opt_left(current_opt_idx, dl, gsl::narrow_cast<float>(dt))) {
      cmdq.pop_front();
    }
    break;

  case commands::shift_opt_right:
    if (do_shift_opt_right(current_opt_idx, dl, gsl::narrow_cast<float>(dt))) {
      cmdq.pop_front();
    }
    break;

  case commands::enter_option:
    do_enter_option(current_opt_idx);
    cmdq.pop_front();
    break;

  default:
    break;
  }
}

void DTU::state::main_menu::keyboard_event(surge::deque<surge::u32> &cmdq, int key, int, int action,

                                           int) noexcept {
  static bool menu_shown{false};

  if (action == GLFW_PRESS && !menu_shown) {
    cmdq.push_back(commands::show_menu);
    menu_shown = true;
    return;
  }

  if (action == GLFW_PRESS && key == GLFW_KEY_LEFT && menu_shown) {
    cmdq.push_back(commands::shift_opt_right);
  }

  if (action == GLFW_PRESS && key == GLFW_KEY_RIGHT && menu_shown) {
    cmdq.push_back(commands::shift_opt_left);
  }

  if (action == GLFW_PRESS && key == GLFW_KEY_ENTER && menu_shown) {
    cmdq.push_back(commands::enter_option);
  }
}