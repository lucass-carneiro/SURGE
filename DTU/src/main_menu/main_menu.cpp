#include "main_menu/main_menu.hpp"

#include "player/logging.hpp"
#include "player/nonuniform_tiles.hpp"
#include "player/static_image.hpp"
#include "player/window.hpp"

#include <array>
#include <glm/gtc/matrix_transform.hpp>
#include <gsl/gsl-lite.hpp>

// NOLINTNEXTLINE
static surge::atom::nonuniform_tiles::buffer_data g_background_buffer{};

// NOLINTNEXTLINE
static surge::atom::nonuniform_tiles::draw_data g_background_draw_data{};

// NOLINTNEXTLINE
static surge::atom::static_image::one_buffer_data g_title_buffer{};

// NOLINTNEXTLINE
static surge::atom::static_image::one_draw_data g_title_draw_data{};

// NOLINTNEXTLINE
static surge::atom::text::draw_data g_text_draw_data{};

// Parallax background
static constexpr const GLsizei background_layer_count{5};

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

auto DTU::state::main_menu::load(surge::queue<surge::u32> &cmdq, float ww, float wh) noexcept
    -> int {
  using namespace surge;

  log_info("Loading main_menu state");

  // Background
  log_info("Loading background images");

  atom::nonuniform_tiles::tile_structure background_ts{"resources/main_menu/background.png",
                                                       background_layer_count};
  auto background_buffer{atom::nonuniform_tiles::create(background_ts)};

  if (!background_buffer) {
    return static_cast<int>(background_buffer.error());
  }
  g_background_buffer = *background_buffer;

  g_background_draw_data.models.reserve(background_layer_count);

  for (surge::usize i = 0; i < background_layer_count; i++) {
    const float z{gsl::narrow_cast<float>(i) / 10.0f};
    g_background_draw_data.models.push_back(glm::scale(
        glm::translate(glm::mat4{1.0f}, glm::vec3{0.0, 0.0, z}), glm::vec3{2 * ww, wh, 1.0f}));
  }

  // Title
  log_info("Loading title image");

  const auto title_buffer{atom::static_image::create("resources/main_menu/title.png",
                                                     renderer::texture_filtering::linear,
                                                     renderer::texture_wrap::clamp_to_border)};
  if (!title_buffer) {
    return static_cast<int>(title_buffer.error());
  }
  g_title_buffer = *title_buffer;

  const float title_width{608.0f};
  const float title_height{174.0f};

  g_title_draw_data.pos = glm::vec3{(ww - title_width) / 2.0f, (wh - title_height) / 2.0f,
                                    background_layer_count / 10.0f};
  g_title_draw_data.scale = glm::vec3{title_width, title_height, 1.0f};
  g_title_draw_data.region_dims = glm::vec2{title_width, title_height};
  g_title_draw_data.region_origin = glm::vec2{0.0};
  g_title_draw_data.alpha = 0.0f;

  // Options
  g_text_draw_data.face_idx = 0;
  g_text_draw_data.position = glm::vec2{100.0};
  g_text_draw_data.scale = 1.0f;
  g_text_draw_data.color = glm::vec3{175.0f / 255.0f, 17.0f / 255.0f, 28.0f / 255.0f};

  // First command
  cmdq.push(commands::show_title);

  return 0;
}

auto DTU::state::main_menu::unload(surge::queue<surge::u32> &cmdq) noexcept -> int {
  using namespace surge;
  log_info("Unloading main_menu state");

  atom::nonuniform_tiles::cleanup(g_background_buffer);
  atom::static_image::cleanup(g_title_buffer);

  while (!cmdq.empty()) {
    cmdq.pop();
  }

  return 0;
}

auto DTU::state::main_menu::draw(const shader_indices &&si,
                                 const surge::atom::text::buffer_data &tbd,
                                 const surge::atom::text::charmap_data &tcd, glm::mat4 &proj,
                                 glm::mat4 &view) noexcept -> int {
  using namespace surge;

  g_background_draw_data.projection = proj;
  g_background_draw_data.view = view;

  g_title_draw_data.projection = proj;
  g_title_draw_data.view = view;

  atom::nonuniform_tiles::draw(si.nonuniform_tiles, g_background_buffer, g_background_draw_data);
  atom::static_image::draw(si.static_image, g_title_buffer, g_title_draw_data);

  g_text_draw_data.projection = proj;
  atom::text::draw(si.text, tbd, tcd, g_text_draw_data, "Hello World!");

  return 0;
}

auto DTU::state::main_menu::update(surge::queue<surge::u32> &cmdq, double dt) noexcept -> int {
  // Background parallax
  static std::array<float, background_layer_count> relative_positions{0};

  for (surge::usize i = 0; auto &model : g_background_draw_data.models) {
    const auto drift_speed{background_drift_speeds[i]};

    if (relative_positions[i] < 0.5f) {
      model = glm::translate(model, glm::vec3{-drift_speed * dt, 0.0f, 0.0f});
      relative_positions[i] += drift_speed * gsl::narrow_cast<float>(dt);
    } else {
      model = glm::translate(model, glm::vec3{0.5f, 0.0f, 0.0f});
      relative_positions[i] = 0;
    }
    i++;
  }

  // Command handler
  switch (cmdq.front()) {

  case commands::show_title:
    if (g_title_draw_data.alpha < 1.0f) {
      g_title_draw_data.alpha += 1.0f * gsl::narrow_cast<float>(dt);
    } else {
      g_title_draw_data.alpha = 1.0;
      cmdq.pop();
    }
    break;

  case commands::shift_title:
    if (g_title_draw_data.pos[1] > 0.0f) {
      g_title_draw_data.pos[1] -= 1000.0f * gsl::narrow_cast<float>(dt);
    } else {
      g_title_draw_data.pos[1] = 0.0;
      cmdq.pop();
      cmdq.push(commands::show_menu);
    }
    break;

  default:
    break;
  }

  return 0;
}

void DTU::state::main_menu::keyboard_event(surge::queue<surge::u32> &cmdq, int /*key*/, int,
                                           int action, int) noexcept {
  static bool title_shifted{false};
  if (action == GLFW_PRESS && !title_shifted) {
    cmdq.push(commands::shift_title);
    title_shifted = true;
  }
}