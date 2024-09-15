#include "sprite_demo.hpp"

#include "surge_core.hpp"

static constexpr surge::usize max_sprites{10};

using tdb_t = surge::gl_atom::texture::database;
using sdb_t = surge::gl_atom::sprite_database::database;
using pv_ubo_t = surge::gl_atom::pv_ubo::buffer;

namespace globals {

static tdb_t tdb{};
static sdb_t sdb{};
static pv_ubo_t pv_ubo{};

} // namespace globals

extern "C" SURGE_MODULE_EXPORT auto on_load() noexcept -> int {
  using namespace surge;

  log_info("Loading Sprite Demo module");

  // PV UBO
  log_info("Creating projection and view matrices");
  const auto dims{window::get_dims()};
  const auto projection{glm::ortho(0.0f, dims[0], dims[1], 0.0f, 0.0f, 1.0f)};
  const auto view{glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                              glm::vec3(0.0f, 1.0f, 0.0f))};

  globals::pv_ubo = gl_atom::pv_ubo::buffer::create();
  globals::pv_ubo.update_all(&projection, &view);

  // Texture database
  globals::tdb = gl_atom::texture::database::create(max_sprites);

  // Sprite database
  gl_atom::sprite_database::database_create_info sdbci{.max_sprites = max_sprites,
                                                       .buffer_redundancy = 3};
  const auto sdb{gl_atom::sprite_database::create(sdbci)};
  if (!sdb) {
    log_error("Unable to initialize sprite database");
    return static_cast<int>(sdb.error());
  } else {
    globals::sdb = *sdb;
  }

  log_info("Loading resources");
  gl_atom::texture::create_info ci{};
  ci.filtering = gl_atom::texture::texture_filtering::nearest;

  globals::tdb.add(ci, "resources/bird_red.png", "resources/bird_yellow.png",
                   "resources/bird_blue.png");

  log_info("Sprite Demo module loaded");
  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto on_unload() noexcept -> int {
  using namespace surge;

  log_info("Unloading Sprite Demo module");

  log_info("Waiting OpenGL idle");
  renderer::gl::wait_idle();

  gl_atom::sprite_database::destroy(globals::sdb);
  globals::tdb.destroy();

  globals::pv_ubo.destroy();

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto draw() noexcept -> int {
  using namespace surge;

  globals::pv_ubo.bind_to_location(2);
  gl_atom::sprite_database::draw(globals::sdb);

  return 0;
}

static inline auto update_bird_flap_animation_frame(float delta_t) noexcept -> glm::vec4 {
  static const std::array<glm::vec4, 4> frame_views{
      glm::vec4{1.0f, 1.0f, 34.0f, 24.0f}, glm::vec4{36.0f, 1.0f, 34.0f, 24.0f},
      glm::vec4{71.0f, 1.0f, 34.0f, 24.0f}, glm::vec4{106.0f, 1.0f, 34.0f, 24.0f}};

  static const float frame_rate{10.0f};
  static const float wait_time{1.0f / frame_rate};

  static surge::u8 frame_idx{0};
  static float elapsed{0.0f};

  if (elapsed > wait_time) {
    frame_idx++;
    frame_idx %= 4;
    elapsed = 0;
  } else {
    elapsed += delta_t;
  }

  return frame_views[frame_idx]; // NOLINT
}

extern "C" SURGE_MODULE_EXPORT auto update(double delta_t) noexcept -> int {
  using namespace surge;
  using namespace gl_atom::sprite_database;

  static const auto r_texture{globals::tdb.find("resources/bird_red.png").value_or(0)};
  static const auto y_texture{globals::tdb.find("resources/bird_yellow.png").value_or(0)};
  static const auto b_texture{globals::tdb.find("resources/bird_blue.png").value_or(0)};

  static const glm::vec2 original_bird_sheet_size{141.0f, 26.0f};

  const auto flap_frame_view{update_bird_flap_animation_frame(static_cast<float>(delta_t))};

  begin_add(globals::sdb);

  add_view(globals::sdb, r_texture, glm::vec2{10.0f}, glm::vec2{100.0f}, 0.1f, flap_frame_view,
           original_bird_sheet_size, 1.0f);
  add_view(globals::sdb, y_texture, glm::vec2{120.0f}, glm::vec2{100.0f}, 0.1f, flap_frame_view,
           original_bird_sheet_size, 1.0f);
  add_view(globals::sdb, b_texture, glm::vec2{220.0f}, glm::vec2{100.0f}, 0.1f, flap_frame_view,
           original_bird_sheet_size, 1.0f);

  return 0;
}

extern "C" SURGE_MODULE_EXPORT void keyboard_event(int, int, int, int) noexcept {}

extern "C" SURGE_MODULE_EXPORT void mouse_button_event(int, int, int) noexcept {}

extern "C" SURGE_MODULE_EXPORT void mouse_scroll_event(double, double) noexcept {}