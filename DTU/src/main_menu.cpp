#include "main_menu.hpp"

#include "player/logging.hpp"
#include "player/tasks.hpp"
#include "player/window.hpp"
#include "ui.hpp"

#include <glm/gtc/matrix_transform.hpp>

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#  include <tracy/TracyOpenGL.hpp>
#endif

auto DTU::state_impl::main_menu::load(tdb_t &tdb) noexcept -> std::optional<surge::error> {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("DTU::main_menu::load()");
#endif

  using surge::atom::texture::create_info;
  using surge::renderer::texture_filtering;
  using surge::renderer::texture_wrap;
  using namespace surge::atom;

  log_info("Loading main_menu state");

  // Background textures
  create_info ci{texture_filtering::nearest, texture_wrap::clamp_to_edge, 1, true};
  tdb.add(ci, "resources/main_menu/1.png", "resources/main_menu/2.png", "resources/main_menu/3.png",
          "resources/main_menu/4.png", "resources/main_menu/5.png");

  // Title
  ci.filtering = texture_filtering::anisotropic;
  ci.mipmap_levels = 4;
  tdb.add(ci, "resources/main_menu/title.png");

  // Options
  tdb.add(ci, "resources/main_menu/main_menu_options_selected.png",
          "resources/main_menu/main_menu_options_released.png",
          "resources/main_menu/main_menu_options_pressed.png",
          "resources/main_menu/main_menu_new_game_selected.png",
          "resources/main_menu/main_menu_new_game_released.png",
          "resources/main_menu/main_menu_new_game_pressed.png",
          "resources/main_menu/main_menu_load_game_selected.png",
          "resources/main_menu/main_menu_load_game_released.png",
          "resources/main_menu/main_menu_load_game_pressed.png",
          "resources/main_menu/main_menu_exit_selected.png",
          "resources/main_menu/main_menu_exit_released.png",
          "resources/main_menu/main_menu_exit_pressed.png");

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

static inline void update_background_parallax(GLFWwindow *window, float dtf, DTU::tdb_t &tdb,
                                              DTU::sdb_t &sdb) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("DTU::main_menu::update_background_parallax()");
#endif

  using namespace surge;
  using namespace surge::atom;

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

  tasks::executor().silent_async([&]() { prallax_drift(drift_0, drift_speed_0, dtf, model_0); });
  tasks::executor().silent_async([&]() { prallax_drift(drift_1, drift_speed_1, dtf, model_1); });
  tasks::executor().silent_async([&]() { prallax_drift(drift_2, drift_speed_2, dtf, model_2); });
  tasks::executor().silent_async([&]() { prallax_drift(drift_3, drift_speed_3, dtf, model_3); });
  tasks::executor().silent_async([&]() { prallax_drift(drift_4, drift_speed_4, dtf, model_4); });

  tasks::executor().wait_for_all();

  sdb.add(handle_0, model_0, 1.0f);
  sdb.add(handle_1, model_1, 1.0f);
  sdb.add(handle_2, model_2, 1.0f);
  sdb.add(handle_3, model_3, 1.0f);
  sdb.add(handle_4, model_4, 1.0f);
}

static void update_menu(GLFWwindow *window, float dt, DTU::tdb_t &tdb, DTU::sdb_t &sdb,
                        DTU::txd_t &) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("DTU::main_menu::update_menu()");
#endif

  using namespace surge;
  using namespace surge::atom;

  // Texture handles
  static const auto title_handle{tdb.find("resources/main_menu/title.png").value_or(0)};

  static const auto new_game_selected_handle{
      tdb.find("resources/main_menu/main_menu_new_game_selected.png").value_or(0)};

  static const auto new_game_released_handle{
      tdb.find("resources/main_menu/main_menu_new_game_released.png").value_or(0)};

  static const auto new_game_pressed_handle{
      tdb.find("resources/main_menu/main_menu_new_game_pressed.png").value_or(0)};

  static const auto load_game_selected_handle{
      tdb.find("resources/main_menu/main_menu_load_game_selected.png").value_or(0)};

  static const auto load_game_released_handle{
      tdb.find("resources/main_menu/main_menu_load_game_released.png").value_or(0)};

  static const auto load_game_pressed_handle{
      tdb.find("resources/main_menu/main_menu_load_game_pressed.png").value_or(0)};

  static const auto options_selected_handle{
      tdb.find("resources/main_menu/main_menu_options_selected.png").value_or(0)};

  static const auto options_released_handle{
      tdb.find("resources/main_menu/main_menu_options_released.png").value_or(0)};

  static const auto options_pressed_handle{
      tdb.find("resources/main_menu/main_menu_options_pressed.png").value_or(0)};

  static const auto exit_selected_handle{
      tdb.find("resources/main_menu/main_menu_exit_selected.png").value_or(0)};

  static const auto exit_released_handle{
      tdb.find("resources/main_menu/main_menu_exit_released.png").value_or(0)};

  static const auto exit_pressed_handle{
      tdb.find("resources/main_menu/main_menu_exit_pressed.png").value_or(0)};

  // Position metrics
  const auto [ww, wh] = window::get_dims(window);
  const glm::vec2 window_dims{ww, wh};

  const glm::vec2 title_size{896.0f, 250.0f};
  const glm::vec2 opts_size{490.0f, 139.0f};

  const float title_proportion{0.25f * wh};
  const float opts_proportion{0.5f * wh};
  const float spacers_proportion{0.25f * wh};

  const float title_scale{title_proportion / title_size[1]};
  const float opts_scale{(opts_proportion / 4.0f) / opts_size[1]};
  const float spacers_size{spacers_proportion / 6.0f};

  const float z_pos{0.5f};

  const glm::vec2 title_pos{(ww - title_size[0] * title_scale) / 2.0f, spacers_size};

  const glm::vec2 new_game_pos{(ww - opts_size[0] * opts_scale) / 2.0f,
                               title_pos[1] + title_size[1] * title_scale + spacers_size};

  const glm::vec2 load_game_pos{(ww - opts_size[0] * opts_scale) / 2.0f,
                                new_game_pos[1] + opts_size[1] * opts_scale + spacers_size};

  const glm::vec2 options_pos{(ww - opts_size[0] * opts_scale) / 2.0f,
                              load_game_pos[1] + opts_size[1] * opts_scale + spacers_size};

  const glm::vec2 exit_pos{(ww - opts_size[0] * opts_scale) / 2.0f,
                           options_pos[1] + opts_size[1] * opts_scale + spacers_size};

  // Dynamic alpha for fade-in effect
  static float alpha{0.0f};

  // UI code
  static DTU::ui::ui_state uist{window, -1, -1};

  const auto model{sprite::place(title_pos, title_size * title_scale, z_pos)};
  sdb.add(title_handle, model, alpha);

  // New game bttn
  DTU::ui::draw_data dd{new_game_pos, opts_size * opts_scale, z_pos, alpha};
  DTU::ui::button_skin skins{new_game_released_handle, new_game_selected_handle,
                             new_game_pressed_handle};

  DTU::ui::button(__COUNTER__, uist, dd, sdb, skins);

  // Load Game bttn
  dd.pos = load_game_pos;
  skins.handle_press = load_game_pressed_handle;
  skins.handle_release = load_game_released_handle;
  skins.handle_select = load_game_selected_handle;

  DTU::ui::button(__COUNTER__, uist, dd, sdb, skins);

  // Options bttn
  dd.pos = options_pos;
  skins.handle_press = options_pressed_handle;
  skins.handle_release = options_released_handle;
  skins.handle_select = options_selected_handle;

  DTU::ui::button(__COUNTER__, uist, dd, sdb, skins);

  // Exit bttn
  dd.pos = exit_pos;
  skins.handle_press = exit_pressed_handle;
  skins.handle_release = exit_released_handle;
  skins.handle_select = exit_selected_handle;

  if (DTU::ui::button(__COUNTER__, uist, dd, sdb, skins)) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  }

  // Update alpha
  if (1.0f - alpha < 1.0e-2) {
    alpha = 1.0f;
  } else {
    alpha += 0.8f * dt;
  }
}

auto DTU::state_impl::main_menu::update(GLFWwindow *window, double dt, tdb_t &tdb, sdb_t &sdb,
                                        txd_t &txd) noexcept -> std::optional<surge::error> {
  const auto dtf{static_cast<float>(dt)};
  update_background_parallax(window, dtf, tdb, sdb);
  update_menu(window, dtf, tdb, sdb, txd);

  return {};
}