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

  auto opt_images{files::load_image("resources/main_menu/new_game.png")};

  if (opt_images) {
    const auto handle{atom::sprite::create_texture(*opt_images)};
    files::free_image(*opt_images);
    sprite_textures.push_back(handle.value_or(0));
    atom::sprite::make_resident(handle.value_or(0));
    sprite_alphas.push_back(0.0f);
  }
}

static void load_options_quads(surge::vector<glm::mat4> &sprite_models, float ww,
                               float wh) noexcept {
  sprite_models.push_back(glm::scale(
      glm::translate(glm::mat4{1.0f},
                     glm::vec3{(ww - 413.0f) / 2.0f, (wh - 174.0f) / 2.0f + 174.0f + 50.0f, 1.0f}),
      glm::vec3{413.0f, 58.0f, 1.0}));
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

auto DTU::state::main_menu::update(surge::queue<surge::u32> &cmdq,
                                   surge::vector<glm::mat4> &sprite_models,
                                   surge::vector<float> &sprite_alphas, double dt) noexcept -> int {

  update_background_quads(sprite_models, dt);

  const auto title_idx{sprite_models.size() - 2};
  const auto opts_idx{sprite_models.size() - 1};

  switch (cmdq.front()) {

  case commands::show_title:
    if (sprite_alphas[title_idx] < 1.0f) {
      sprite_alphas[title_idx] += 0.5f * dt;
    } else {
      sprite_alphas[title_idx] = 1.0f;
      cmdq.push(commands::show_menu);
      cmdq.pop();
    }
    break;

  case commands::show_menu:
    sprite_alphas[opts_idx] = 1.0f;
    cmdq.pop();
    break;

  default:
    break;
  }

  return 0;
}

void DTU::state::main_menu::keyboard_event(surge::queue<surge::u32> &, int, int, int,
                                           int) noexcept {
  // todo
}