#include "sprite_demo.hpp"

#include "surge_core.hpp"

static constexpr surge::usize max_sprites{10};

using tdb_t = surge::gl_atom::texture::database;
using sdb_t = surge::gl_atom::sprite2::database;
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
  gl_atom::sprite2::database_create_info sdbci{.max_sprites = max_sprites, .buffer_redundancy = 3};
  const auto sdb{gl_atom::sprite2::database_create(sdbci)};
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

  gl_atom::sprite2::database_destroy(globals::sdb);
  globals::tdb.destroy();

  globals::pv_ubo.destroy();

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto draw() noexcept -> int {
  using namespace surge;

  globals::pv_ubo.bind_to_location(2);
  gl_atom::sprite2::database_draw(globals::sdb);

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto update(double) noexcept -> int {
  using namespace surge;

  gl_atom::sprite2::database_begin_add(globals::sdb);
  gl_atom::sprite2::database_add(
      globals::sdb, 0, gl_atom::sprite::place(glm::vec2{10.0f}, glm::vec2{100.0f}, 0.1f), 1.0f);

  return 0;
}

extern "C" SURGE_MODULE_EXPORT void keyboard_event(int, int, int, int) noexcept {}

extern "C" SURGE_MODULE_EXPORT void mouse_button_event(int, int, int) noexcept {}

extern "C" SURGE_MODULE_EXPORT void mouse_scroll_event(double, double) noexcept {}