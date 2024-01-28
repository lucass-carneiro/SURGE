#include "main_menu/main_menu.hpp"

#include "player/files.hpp"
#include "player/logging.hpp"
#include "player/sprite.hpp"
#include "player/window.hpp"

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

static void load_background_images(surge::vector<GLuint64> &sprite_textures,
                                   surge::vector<float> &sprite_alphas) noexcept {
  using namespace surge;

  log_info("Loading background images");

  const std::array<const char *, background_layer_count> background_image_names{
      "resources/main_menu/1.png", "resources/main_menu/2.png", "resources/main_menu/3.png",
      "resources/main_menu/4.png", "resources/main_menu/5.png"};

  std::array<tl::expected<files::image, error>, background_layer_count> background_images{};
  std::array<tl::expected<GLuint64, error>, background_layer_count> background_handles{};

  // Load images
  for (usize i = 0; i < background_layer_count; i++) {
    background_images[i] = files::load_image(background_image_names[i]);
  }

  // Create handles for valid images
  for (usize i = 0; auto &img : background_images) {
    if (img) {
      background_handles[i]
          = atom::sprite::create_texture(*img, renderer::texture_filtering::nearest);
      files::free_image(*img);
    }
    i++;
  }

  // Push valid handles to draw list and make textures resident
  for (const auto &handle : background_handles) {
    sprite_textures.push_back(handle.value_or(0));
    atom::sprite::make_resident(handle.value_or(0));
    sprite_alphas.push_back(1.0f);
  }
}

static void load_background_quads(surge::vector<glm::mat4> &sprite_models, float ww,
                                  float wh) noexcept {
  using namespace surge;

  for (usize i = 0; i < background_layer_count; i++) {
    sprite_models.push_back(
        glm::scale(glm::translate(glm::mat4{1.0f}, glm::vec3{0.0f, 0.0f, 0.0f + i / 10.0f}),
                   glm::vec3{ww * 2.0f, wh, 1.0}));
  }
}

static void update_background_quads(surge::vector<glm::mat4> &sprite_models, double dt) noexcept {
  using namespace surge;

  static std::array<float, background_layer_count> relative_positions{0};
  const auto dtf{gsl::narrow_cast<float>(dt)};

  for (usize i = 0; auto &model : std::span(sprite_models).first(background_layer_count)) {
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

static void load_title_image(surge::vector<GLuint64> &sprite_textures,
                             surge::vector<float> &sprite_alphas) noexcept {
  using namespace surge;

  log_info("Loading title images");

  auto title_image{files::load_image("resources/main_menu/title.png")};

  if (title_image) {
    const auto handle{atom::sprite::create_texture(*title_image)};
    files::free_image(*title_image);
    sprite_textures.push_back(handle.value_or(0));
    atom::sprite::make_resident(handle.value_or(0));
    sprite_alphas.push_back(0.0f);
  }
}

static void load_title_quad(surge::vector<glm::mat4> &sprite_models, float ww, float wh) noexcept {
  sprite_models.push_back(glm::scale(
      glm::translate(glm::mat4{1.0f}, glm::vec3{(ww - 608.0f) / 2.0f, (wh - 174.0f) / 2.0f, 0.5f}),
      glm::vec3{608.0f, 174.0f, 1.0}));
}

static void load_options_images(surge::vector<GLuint64> &sprite_textures,
                                surge::vector<float> &sprite_alphas) noexcept {
  using namespace surge;

  log_info("Loading options images");

  auto opt_img_new{files::load_image("resources/main_menu/menu_new_game.png")};
  auto opt_img_load{files::load_image("resources/main_menu/menu_load_game.png")};
  auto opt_img_options{files::load_image("resources/main_menu/menu_options.png")};
  auto border_credits{files::load_image("resources/main_menu/menu_credits.png")};
  auto border_exit{files::load_image("resources/main_menu/menu_exit.png")};
  auto border_img{files::load_image("resources/main_menu/menu_border.png")};

  if (opt_img_new) {
    const auto handle{atom::sprite::create_texture(*opt_img_new)};
    files::free_image(*opt_img_new);
    sprite_textures.push_back(handle.value_or(0));
    atom::sprite::make_resident(handle.value_or(0));
    sprite_alphas.push_back(0.0f);
  }

  if (opt_img_load) {
    const auto handle{atom::sprite::create_texture(*opt_img_load)};
    files::free_image(*opt_img_load);
    sprite_textures.push_back(handle.value_or(0));
    atom::sprite::make_resident(handle.value_or(0));
    sprite_alphas.push_back(0.0f);
  }

  if (opt_img_options) {
    const auto handle{atom::sprite::create_texture(*opt_img_options)};
    files::free_image(*opt_img_options);
    sprite_textures.push_back(handle.value_or(0));
    atom::sprite::make_resident(handle.value_or(0));
    sprite_alphas.push_back(0.0f);
  }

  if (border_credits) {
    const auto handle{atom::sprite::create_texture(*border_credits)};
    files::free_image(*border_credits);
    sprite_textures.push_back(handle.value_or(0));
    atom::sprite::make_resident(handle.value_or(0));
    sprite_alphas.push_back(0.0f);
  }

  if (border_exit) {
    const auto handle{atom::sprite::create_texture(*border_exit)};
    files::free_image(*border_exit);
    sprite_textures.push_back(handle.value_or(0));
    atom::sprite::make_resident(handle.value_or(0));
    sprite_alphas.push_back(0.0f);
  }

  if (border_img) {
    const auto handle{atom::sprite::create_texture(*border_img)};
    files::free_image(*border_img);
    sprite_textures.push_back(handle.value_or(0));
    atom::sprite::make_resident(handle.value_or(0));
    sprite_alphas.push_back(0.0f);
  }
}

static void load_options_quads(surge::vector<glm::mat4> &sprite_models, float ww,
                               float wh) noexcept {

  sprite_models.push_back(glm::scale(
      glm::translate(glm::mat4{1.0f},
                     glm::vec3{(ww - 448.0f) / 2.0f, (wh - 133.0f) / 2.0f + 133.0f + 50.0f, 0.6f}),
      glm::vec3{448.0f, 133.0f, 1.0}));

  sprite_models.push_back(glm::scale(
      glm::translate(glm::mat4{1.0f}, glm::vec3{(ww - 448.0f) / 2.0f + 448.0f,
                                                (wh - 133.0f) / 2.0f + 133.0f + 50.0f, 0.6f}),
      glm::vec3{448.0f, 133.0f, 1.0}));

  sprite_models.push_back(glm::scale(
      glm::translate(glm::mat4{1.0f}, glm::vec3{(ww - 448.0f) / 2.0f + 448.0f,
                                                (wh - 133.0f) / 2.0f + 133.0f + 50.0f, 0.6f}),
      glm::vec3{448.0f, 133.0f, 1.0}));

  sprite_models.push_back(glm::scale(
      glm::translate(glm::mat4{1.0f}, glm::vec3{(ww - 448.0f) / 2.0f + 448.0f,
                                                (wh - 133.0f) / 2.0f + 133.0f + 50.0f, 0.6f}),
      glm::vec3{448.0f, 133.0f, 1.0}));

  sprite_models.push_back(glm::scale(
      glm::translate(glm::mat4{1.0f}, glm::vec3{(ww - 448.0f) / 2.0f + 448.0f,
                                                (wh - 133.0f) / 2.0f + 133.0f + 50.0f, 0.6f}),
      glm::vec3{448.0f, 133.0f, 1.0}));

  sprite_models.push_back(glm::scale(
      glm::translate(glm::mat4{1.0f},
                     glm::vec3{(ww - 448.0f) / 2.0f, (wh - 133.0f) / 2.0f + 133.0f + 50.0f, 0.7f}),
      glm::vec3{448.0f, 133.0f, 1.0}));
}

auto DTU::state::main_menu::load(surge::queue<surge::u32> &cmdq,
                                 surge::vector<glm::mat4> &sprite_models,
                                 surge::vector<GLuint64> &sprite_textures,
                                 surge::vector<float> &sprite_alphas, float ww, float wh) noexcept
    -> int {
  using namespace surge;

  log_info("Loading main_menu state");

  // Background
  load_background_images(sprite_textures, sprite_alphas);
  load_background_quads(sprite_models, ww, wh);

  // Title
  load_title_image(sprite_textures, sprite_alphas);
  load_title_quad(sprite_models, ww, wh);

  // Options
  load_options_images(sprite_textures, sprite_alphas);
  load_options_quads(sprite_models, ww, wh);

  // First command
  cmdq.push(commands::show_title);

  return 0;
}

auto DTU::state::main_menu::unload(surge::queue<surge::u32> &,
                                   surge::vector<glm::mat4> &sprite_models,
                                   surge::vector<GLuint64> &sprite_textures) noexcept -> int {
  using namespace surge;

  log_info("Unloading main_menu state");

  atom::sprite::make_non_resident(sprite_textures);
  sprite_textures.clear();
  sprite_models.clear();

  return 0;
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

static auto do_shift_opt_left(surge::usize &current_opt_idx,
                              surge::vector<glm::mat4> &sprite_models,
                              surge::vector<float> &sprite_alphas, float dt) noexcept -> bool {

  // Do not shift past the exit option
  const auto next_opt_index{current_opt_idx + 1};

  if (next_opt_index > 10) {
    return true;
  }

  const float speed{1.5f * dt};

  bool shift_opts{false};
  bool fade_current_opt{false};
  bool unfade_next_opt{false};

  static const auto opt_initial_x_pos{sprite_models[current_opt_idx][3][0]};

  if (sprite_models[current_opt_idx][3][0] > (opt_initial_x_pos - 448.0f)) {
    sprite_models[current_opt_idx]
        = glm::translate(sprite_models[current_opt_idx], glm::vec3{-speed, 0.0f, 0.0f});
    sprite_models[next_opt_index]
        = glm::translate(sprite_models[next_opt_index], glm::vec3{-speed, 0.0f, 0.0f});
  } else {
    sprite_models[current_opt_idx][3][0] = opt_initial_x_pos - 448.0f;
    sprite_models[next_opt_index][3][0] = opt_initial_x_pos;
    shift_opts = true;
  }

  if (sprite_alphas[current_opt_idx] > 0.0f) {
    sprite_alphas[current_opt_idx] -= speed;
  } else {
    sprite_alphas[current_opt_idx] = 0.0f;
    fade_current_opt = true;
  }

  if (sprite_alphas[next_opt_index] < 1.0f) {
    sprite_alphas[next_opt_index] += speed;
  } else {
    sprite_alphas[next_opt_index] = 1.0f;
    unfade_next_opt = true;
  }

  if (shift_opts && fade_current_opt && unfade_next_opt) {
    current_opt_idx = next_opt_index;
    return true;
  } else {
    return false;
  }
}

static auto do_shift_opt_right(surge::usize &current_opt_idx,
                               surge::vector<glm::mat4> &sprite_models,
                               surge::vector<float> &sprite_alphas, float dt) noexcept -> bool {

  // Do not shift past the exit new game
  const auto next_opt_index{current_opt_idx - 1};

  if (next_opt_index < 6) {
    return true;
  }

  const float speed{1.5f * dt};

  bool shift_opts{false};
  bool fade_current_opt{false};
  bool unfade_next_opt{false};

  static const auto opt_initial_x_pos{sprite_models[current_opt_idx][3][0]};

  if (sprite_models[current_opt_idx][3][0] < (opt_initial_x_pos + 448.0f)) {
    sprite_models[current_opt_idx]
        = glm::translate(sprite_models[current_opt_idx], glm::vec3{speed, 0.0f, 0.0f});
    sprite_models[next_opt_index]
        = glm::translate(sprite_models[next_opt_index], glm::vec3{speed, 0.0f, 0.0f});
  } else {
    sprite_models[current_opt_idx][3][0] = opt_initial_x_pos + 448.0f;
    sprite_models[next_opt_index][3][0] = opt_initial_x_pos;
    shift_opts = true;
  }

  if (sprite_alphas[current_opt_idx] > 0.0f) {
    sprite_alphas[current_opt_idx] -= speed;
  } else {
    sprite_alphas[current_opt_idx] = 0.0f;
    fade_current_opt = true;
  }

  if (sprite_alphas[next_opt_index] < 1.0f) {
    sprite_alphas[next_opt_index] += speed;
  } else {
    sprite_alphas[next_opt_index] = 1.0f;
    unfade_next_opt = true;
  }

  if (shift_opts && fade_current_opt && unfade_next_opt) {
    current_opt_idx = next_opt_index;
    return true;
  } else {
    return false;
  }
}

void do_enter_option(surge::queue<surge::u32> &cmdq, surge::usize current_opt_idx) noexcept {
  switch (current_opt_idx) {
  case 10:
    log_info("opt_exit selected");
    cmdq.push(DTU::state::main_menu::commands::exit_game);
    break;

  default:
    break;
  }
}

auto DTU::state::main_menu::update(surge::queue<surge::u32> &cmdq,
                                   surge::vector<glm::mat4> &sprite_models,
                                   surge::vector<float> &sprite_alphas, double dt) noexcept -> int {

  update_background_quads(sprite_models, dt);

  const entity_indices ei{sprite_models.size() - 7, sprite_models.size() - 6,
                          sprite_models.size() - 5, sprite_models.size() - 4,
                          sprite_models.size() - 3, sprite_models.size() - 2,
                          sprite_models.size() - 1};

  static auto current_opt_idx{ei.new_game_idx};

  switch (cmdq.size() == 0 ? commands::idle : cmdq.front()) {

  case commands::show_title:
    if (sprite_alphas[ei.title_idx] < 1.0f) {
      sprite_alphas[ei.title_idx] += 1.0f * gsl::narrow_cast<float>(dt);
    } else {
      sprite_alphas[ei.title_idx] = 1.0f;
      cmdq.pop();
    }
    break;

  case commands::show_menu:
    if (sprite_alphas[ei.border_idx] < 1.0f) {
      sprite_alphas[ei.border_idx] += 1.0f * gsl::narrow_cast<float>(dt);
      sprite_alphas[ei.new_game_idx] += 1.0f * gsl::narrow_cast<float>(dt);
    } else {
      sprite_alphas[ei.border_idx] = 1.0f;
      sprite_alphas[ei.new_game_idx] = 1.0f;
      cmdq.pop();
    }
    break;

  case commands::shift_opt_left:
    if (do_shift_opt_left(current_opt_idx, sprite_models, sprite_alphas,
                          gsl::narrow_cast<float>(dt))) {
      cmdq.pop();
    }
    break;

  case commands::shift_opt_right:
    if (do_shift_opt_right(current_opt_idx, sprite_models, sprite_alphas,
                           gsl::narrow_cast<float>(dt))) {
      cmdq.pop();
    }
    break;

  case commands::enter_option:
    do_enter_option(cmdq, current_opt_idx);
    cmdq.pop();
    break;

  case commands::exit_game:
    return surge::error::normal_exit;
    break;

  default:
    break;
  }

  return 0;
}

void DTU::state::main_menu::keyboard_event(surge::queue<surge::u32> &cmdq, int key, int, int action,

                                           int) noexcept {
  static bool menu_shown{false};

  if (action == GLFW_PRESS && !menu_shown) {
    cmdq.push(commands::show_menu);
    menu_shown = true;
    return;
  }

  if (action == GLFW_PRESS && key == GLFW_KEY_LEFT && menu_shown) {
    cmdq.push(commands::shift_opt_right);
  }

  if (action == GLFW_PRESS && key == GLFW_KEY_RIGHT && menu_shown) {
    cmdq.push(commands::shift_opt_left);
  }

  if (action == GLFW_PRESS && key == GLFW_KEY_ENTER && menu_shown) {
    cmdq.push(commands::enter_option);
  }
}