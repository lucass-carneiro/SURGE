#include "main_menu.hpp"

#include "player/logging.hpp"
#include "player/window.hpp"

#include <glm/gtc/matrix_transform.hpp>

auto DTU::state_impl::main_menu::load(tdb_t &tdb) noexcept -> std::optional<surge::error> {
  using surge::atom::texture::create_info;
  using surge::renderer::texture_filtering;
  using surge::renderer::texture_wrap;
  using namespace surge::atom;

  log_info("Loading main_menu state");

  // Background textures
  create_info ci{texture_filtering::nearest, texture_wrap::clamp_to_edge, 1, true};
  tdb.add(ci, "resources/main_menu/1.png", "resources/main_menu/2.png", "resources/main_menu/3.png",
          "resources/main_menu/4.png", "resources/main_menu/5.png");

  return {};
}

auto DTU::state_impl::main_menu::unload(tdb_t &tdb) noexcept -> std::optional<surge::error> {
  log_info("Unloading main_menu state");

  tdb.reset();

  return {};
}

static inline void prallax_drift(float &drift, float drift_speed, float dtf,
                                 glm::mat4 &model) noexcept {
  if (drift < 0.5f) {
    model = glm::translate(model, glm::vec3{-drift_speed * dtf, 0.0f, 0.0f});
    drift += drift_speed * dtf;
  } else {
    model = glm::translate(model, glm::vec3{0.5f, 0.0f, 0.0f});
    drift = 0;
  }
}

static inline void update_background_parallax(GLFWwindow *window, double dt, DTU::tdb_t &tdb,
                                              DTU::sdb_t &sdb) noexcept {
  using namespace surge;
  using namespace surge::atom;

  const auto dtf{static_cast<float>(dt)};
  const auto [ww, wh] = window::get_dims(window);

  // Background texture handles
  static const auto handle_0{tdb.find("resources/main_menu/1.png").value_or(0)};
  static const auto handle_1{tdb.find("resources/main_menu/2.png").value_or(0)};
  static const auto handle_2{tdb.find("resources/main_menu/3.png").value_or(0)};
  static const auto handle_3{tdb.find("resources/main_menu/4.png").value_or(0)};
  static const auto handle_4{tdb.find("resources/main_menu/5.png").value_or(0)};

  // Background parallax layer speeds
  // Obtained from the cubic polynomial (4 + x + x^2) / 400
  static constexpr float drift_speed_0{1.0f / 100.0f};
  static constexpr float drift_speed_1{3.0f / 200.0f};
  static constexpr float drift_speed_2{1.0f / 40.0f};
  static constexpr float drift_speed_3{1.0f / 25.0f};
  static constexpr float drift_speed_4{3.0f / 50.0f};

  // Background model matrices
  static auto model_0{sprite::place(glm::vec2{0.0f}, glm::vec2{2.0f * ww, wh}, 0.0f)};
  static auto model_1{sprite::place(glm::vec2{0.0f}, glm::vec2{2.0f * ww, wh}, 0.1f)};
  static auto model_2{sprite::place(glm::vec2{0.0f}, glm::vec2{2.0f * ww, wh}, 0.2f)};
  static auto model_3{sprite::place(glm::vec2{0.0f}, glm::vec2{2.0f * ww, wh}, 0.3f)};
  static auto model_4{sprite::place(glm::vec2{0.0f}, glm::vec2{2.0f * ww, wh}, 0.4f)};

  // Parallax Drifts
  static float drift_0{0.0};
  static float drift_1{0.0};
  static float drift_2{0.0};
  static float drift_3{0.0};
  static float drift_4{0.0};

  prallax_drift(drift_0, drift_speed_0, dtf, model_0);
  prallax_drift(drift_1, drift_speed_1, dtf, model_1);
  prallax_drift(drift_2, drift_speed_2, dtf, model_2);
  prallax_drift(drift_3, drift_speed_3, dtf, model_3);
  prallax_drift(drift_4, drift_speed_4, dtf, model_4);

  sdb.add(handle_0, model_0, 1.0f);
  sdb.add(handle_1, model_1, 1.0f);
  sdb.add(handle_2, model_2, 1.0f);
  sdb.add(handle_3, model_3, 1.0f);
  sdb.add(handle_4, model_4, 1.0f);
}

auto DTU::state_impl::main_menu::update(GLFWwindow *window, double dt, tdb_t &tdb,
                                        sdb_t &sdb) noexcept -> std::optional<surge::error> {
  update_background_parallax(window, dt, tdb, sdb);
  return {};
}