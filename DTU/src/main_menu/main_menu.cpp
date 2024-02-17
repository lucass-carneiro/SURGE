// clang-format off
#include "DTU.hpp"

#include "main_menu/main_menu.hpp"
#include "states.hpp"

#include "player/logging.hpp"
#include "player/sprite.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <gsl/gsl-lite.hpp>

#include <array>
#include <span>

// clang-format on

// Parallax background layer count
static constexpr const GLsizei background_layer_count{5};

// clang-format off
// Parallax layer speeds
// Obtained from the cubic polynomial (4 + x + x^2) / 400
static constexpr const std::array<float, background_layer_count> background_drift_speeds{
    1.0f / 100.0f,
    3.0f / 200.0f,
    1.0f / 40.0f,
    1.0f / 25.0f,
    3.0f / 50.0f
};
// clang-format on

static void update_background_quads(DTU::sdl_t &sdl, double dt) noexcept {
  using namespace surge;

  static std::array<float, background_layer_count> relative_positions{0};
  const auto dtf{gsl::narrow_cast<float>(dt)};

  for (surge::usize i = 0; auto &model : std::span(sdl.models).first(background_layer_count)) {
    const auto drift_speed{background_drift_speeds[i]}; // NOLINT

    // NOLINTNEXTLINE
    if (relative_positions[i] < 0.5f) {
      model = glm::translate(model, glm::vec3{-drift_speed * dtf, 0.0f, 0.0f});
      relative_positions[i] += drift_speed * dtf; // NOLINT
    } else {
      model = glm::translate(model, glm::vec3{0.5f, 0.0f, 0.0f});
      relative_positions[i] = 0; // NOLINT
    }
    i++;
  }
}

static auto do_shift_opt_left(surge::usize &current_opt_idx, DTU::sdl_t &sdl, float dt) noexcept
    -> bool {

  // Do not shift past the exit option
  const auto next_opt_index{current_opt_idx + 1};

  if (next_opt_index > 10) {
    return true;
  }

  const float speed{1.5f * dt};

  bool shift_opts{false};
  bool fade_current_opt{false};
  bool unfade_next_opt{false};

  static const auto opt_initial_x_pos{sdl.models[current_opt_idx][3][0]};

  if (sdl.models[current_opt_idx][3][0] > (opt_initial_x_pos - 448.0f)) {
    sdl.models[current_opt_idx]
        = glm::translate(sdl.models[current_opt_idx], glm::vec3{-speed, 0.0f, 0.0f});
    sdl.models[next_opt_index]
        = glm::translate(sdl.models[next_opt_index], glm::vec3{-speed, 0.0f, 0.0f});
  } else {
    sdl.models[current_opt_idx][3][0] = opt_initial_x_pos - 448.0f;
    sdl.models[next_opt_index][3][0] = opt_initial_x_pos;
    shift_opts = true;
  }

  if (sdl.alphas[current_opt_idx] > 0.0f) {
    sdl.alphas[current_opt_idx] -= speed;
  } else {
    sdl.alphas[current_opt_idx] = 0.0f;
    fade_current_opt = true;
  }

  if (sdl.alphas[next_opt_index] < 1.0f) {
    sdl.alphas[next_opt_index] += speed;
  } else {
    sdl.alphas[next_opt_index] = 1.0f;
    unfade_next_opt = true;
  }

  if (shift_opts && fade_current_opt && unfade_next_opt) {
    current_opt_idx = next_opt_index;
    return true;
  } else {
    return false;
  }
}

static auto do_shift_opt_right(surge::usize &current_opt_idx, DTU::sdl_t &sdl, float dt) noexcept
    -> bool {

  // Do not shift past the exit new game
  const auto next_opt_index{current_opt_idx - 1};

  if (next_opt_index < 6) {
    return true;
  }

  const float speed{1.5f * dt};

  bool shift_opts{false};
  bool fade_current_opt{false};
  bool unfade_next_opt{false};

  static const auto opt_initial_x_pos{sdl.models[current_opt_idx][3][0]};

  if (sdl.models[current_opt_idx][3][0] < (opt_initial_x_pos + 448.0f)) {
    sdl.models[current_opt_idx]
        = glm::translate(sdl.models[current_opt_idx], glm::vec3{speed, 0.0f, 0.0f});
    sdl.models[next_opt_index]
        = glm::translate(sdl.models[next_opt_index], glm::vec3{speed, 0.0f, 0.0f});
  } else {
    sdl.models[current_opt_idx][3][0] = opt_initial_x_pos + 448.0f;
    sdl.models[next_opt_index][3][0] = opt_initial_x_pos;
    shift_opts = true;
  }

  if (sdl.alphas[current_opt_idx] > 0.0f) {
    sdl.alphas[current_opt_idx] -= speed;
  } else {
    sdl.alphas[current_opt_idx] = 0.0f;
    fade_current_opt = true;
  }

  if (sdl.alphas[next_opt_index] < 1.0f) {
    sdl.alphas[next_opt_index] += speed;
  } else {
    sdl.alphas[next_opt_index] = 1.0f;
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

auto DTU::state::main_menu::load(DTU::cmdq_t &cmdq, vec_glui &ids, vec_glui64 &handles, sdl_t &sdl,
                                 float ww, float wh) noexcept -> int {
  log_info("Loading main_menu state");

  // Background

  load_push_sprite(ids, handles, "resources/main_menu/1.png", sdl,
                   make_model(glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{ww * 2.0f, wh, 1.0}), 1.0);

  load_push_sprite(ids, handles, "resources/main_menu/2.png", sdl,
                   make_model(glm::vec3{0.0f, 0.0f, 0.1f}, glm::vec3{ww * 2.0f, wh, 1.0}), 1.0);

  load_push_sprite(ids, handles, "resources/main_menu/3.png", sdl,
                   make_model(glm::vec3{0.0f, 0.0f, 0.2f}, glm::vec3{ww * 2.0f, wh, 1.0}), 1.0);

  load_push_sprite(ids, handles, "resources/main_menu/4.png", sdl,
                   make_model(glm::vec3{0.0f, 0.0f, 0.3f}, glm::vec3{ww * 2.0f, wh, 1.0}), 1.0);

  load_push_sprite(ids, handles, "resources/main_menu/5.png", sdl,
                   make_model(glm::vec3{0.0f, 0.0f, 0.4f}, glm::vec3{ww * 2.0f, wh, 1.0}), 1.0);

  // Title
  load_push_sprite(ids, handles, "resources/main_menu/title.png", sdl,
                   make_model(glm::vec3{(ww - 608.0f) / 2.0f, (wh - 174.0f) / 2.0f, 0.6f},
                              glm::vec3{608.0f, 174.0f, 1.0}),
                   0.0);

  load_push_sprite(
      ids, handles, "resources/main_menu/menu_new_game.png", sdl,
      make_model(glm::vec3{(ww - 448.0f) / 2.0f, (wh - 133.0f) / 2.0f + 133.0f + 50.0f, 0.6f},
                 glm::vec3{448.0f, 133.0f, 1.0}),
      0.0);

  load_push_sprite(ids, handles, "resources/main_menu/menu_load_game.png", sdl,
                   make_model(glm::vec3{(ww - 448.0f) / 2.0f + 448.0f,
                                        (wh - 133.0f) / 2.0f + 133.0f + 50.0f, 0.6f},
                              glm::vec3{448.0f, 133.0f, 1.0}),
                   0.0);

  load_push_sprite(ids, handles, "resources/main_menu/menu_options.png", sdl,
                   make_model(glm::vec3{(ww - 448.0f) / 2.0f + 448.0f,
                                        (wh - 133.0f) / 2.0f + 133.0f + 50.0f, 0.6f},
                              glm::vec3{448.0f, 133.0f, 1.0}),
                   0.0);

  load_push_sprite(ids, handles, "resources/main_menu/menu_credits.png", sdl,
                   make_model(glm::vec3{(ww - 448.0f) / 2.0f + 448.0f,
                                        (wh - 133.0f) / 2.0f + 133.0f + 50.0f, 0.6f},
                              glm::vec3{448.0f, 133.0f, 1.0}),
                   0.0);

  load_push_sprite(ids, handles, "resources/main_menu/menu_exit.png", sdl,
                   make_model(glm::vec3{(ww - 448.0f) / 2.0f + 448.0f,
                                        (wh - 133.0f) / 2.0f + 133.0f + 50.0f, 0.6f},
                              glm::vec3{448.0f, 133.0f, 1.0}),
                   0.0);

  load_push_sprite(
      ids, handles, "resources/main_menu/menu_border.png", sdl,
      make_model(glm::vec3{(ww - 448.0f) / 2.0f, (wh - 133.0f) / 2.0f + 133.0f + 50.0f, 0.7f},
                 glm::vec3{448.0f, 133.0f, 1.0}),
      0.0);

  surge::atom::sprite::make_resident(handles);

  // First command
  cmdq.push_back(commands::show_title);

  return 0;
}

void DTU::state::main_menu::unload(cmdq_t &cmdq, vec_glui &ids, vec_glui64 &handles,
                                   sdl_t &sdl) noexcept {
  log_info("Unloading main_menu state");

  unload_textures(ids, handles);
  clear_sprites(sdl);

  cmdq.clear();
}

void DTU::state::main_menu::update(cmdq_t &cmdq, sdl_t &sdl, double dt) noexcept {
  update_background_quads(sdl, dt);

  const auto title_idx{sdl.models.size() - 7};
  const auto new_game_idx{sdl.models.size() - 6};
  //   const auto load_game_idx{sdl.models.size() - 5};
  //   const auto options_index{sdl.models.size() - 4};
  //   const auto credits_idx{sdl.models.size() - 3};
  //   const auto exit_idx{sdl.models.size() - 2};
  const auto border_idx{sdl.models.size() - 1};

  static auto current_opt_idx{new_game_idx};

  switch (cmdq.size() == 0 ? commands::idle : cmdq.front()) {

  case commands::show_title:
    if (sdl.alphas[title_idx] < 1.0f) {
      sdl.alphas[title_idx] += 1.0f * gsl::narrow_cast<float>(dt);
    } else {
      sdl.alphas[title_idx] = 1.0f;
      cmdq.pop_front();
    }
    break;

  case commands::show_menu:
    if (sdl.alphas[border_idx] < 1.0f) {
      sdl.alphas[border_idx] += 1.0f * gsl::narrow_cast<float>(dt);
      sdl.alphas[new_game_idx] += 1.0f * gsl::narrow_cast<float>(dt);
    } else {
      sdl.alphas[border_idx] = 1.0f;
      sdl.alphas[new_game_idx] = 1.0f;
      cmdq.pop_front();
    }
    break;

  case commands::shift_opt_left:
    if (do_shift_opt_left(current_opt_idx, sdl, gsl::narrow_cast<float>(dt))) {
      cmdq.pop_front();
    }
    break;

  case commands::shift_opt_right:
    if (do_shift_opt_right(current_opt_idx, sdl, gsl::narrow_cast<float>(dt))) {
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

void DTU::state::main_menu::keyboard_event(cmdq_t &cmdq, int key, int, int action, int) noexcept {
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