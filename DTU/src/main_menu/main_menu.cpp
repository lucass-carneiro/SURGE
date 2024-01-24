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

static void load_background_images(surge::vector<GLuint64> &sprite_textures) noexcept {
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

auto DTU::state::main_menu::load(surge::queue<surge::u32> &,
                                 surge::vector<glm::mat4> &sprite_models,
                                 surge::vector<GLuint64> &sprite_textures, float ww,
                                 float wh) noexcept -> int {
  using namespace surge;

  log_info("Loading main_menu state");

  // Background
  load_background_images(sprite_textures);
  load_background_quads(sprite_models, ww, wh);

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

auto DTU::state::main_menu::update(surge::queue<surge::u32> &,
                                   surge::vector<glm::mat4> &sprite_models, double dt) noexcept
    -> int {
  update_background_quads(sprite_models, dt);
  return 0;
}

void DTU::state::main_menu::keyboard_event(surge::queue<surge::u32> &, int /*key*/, int, int,
                                           int) noexcept {
  // TODO
}