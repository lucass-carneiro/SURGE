#include "main_menu/main_menu.hpp"

#include "player/logging.hpp"
#include "player/nonuniform_tiles.hpp"

#include <array>
#include <glm/gtc/matrix_transform.hpp>
#include <gsl/gsl-lite.hpp>

// NOLINTNEXTLINE
static surge::atom::nonuniform_tiles::buffer_data g_background_buffer{};
static surge::atom::nonuniform_tiles::draw_data g_background_draw_data{};

static constexpr const GLsizei background_layer_count{5};
static constexpr const std::array<float, background_layer_count> background_drift_speeds{
    0.0f, 0.01f, 0.02f, 0.04f, 0.08f};

auto DTU::state::main_menu::load(float ww, float wh) noexcept -> int {
  using namespace surge;

  log_info("Loading main_menu state");

  // Load background
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

  return 0;
}

auto DTU::state::main_menu::unload() noexcept -> int {
  using namespace surge;
  log_info("Unloading main_menu state");

  atom::nonuniform_tiles::cleanup(g_background_buffer);

  return 0;
}

auto DTU::state::main_menu::draw(unsigned int nuts, glm::mat4 &proj, glm::mat4 &view) noexcept
    -> int {
  using namespace surge;

  g_background_draw_data.projection = proj;
  g_background_draw_data.view = view;
  atom::nonuniform_tiles::draw(nuts, g_background_buffer, g_background_draw_data);
  return 0;
}

auto DTU::state::main_menu::update(double dt) noexcept -> int {
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

  return 0;
}