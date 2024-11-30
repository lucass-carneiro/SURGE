#include "text_demo.hpp"

#include "sc_glm_includes.hpp"
#include "sc_logging.hpp"
#include "sc_opengl/atoms/pv_ubo.hpp"
#include "sc_opengl/atoms/text_database.hpp"
#include "sc_opengl/sc_opengl.hpp"
#include "sc_window.hpp"

#include <array>

using txtgc_t = surge::gl_atom::text_database::glyph_cache::cache;
using pv_ubo_t = surge::gl_atom::pv_ubo::buffer;

namespace globals {

static txtgc_t txt_gc{};
static pv_ubo_t pv_ubo{};

} // namespace globals

extern "C" SURGE_MODULE_EXPORT auto on_load() -> int {
  using namespace surge;
  using namespace gl_atom;

  log_info("Loading Text Demo module");

  // PV UBO
  log_info("Creating projection and view matrices");
  const auto dims{window::get_dims()};
  const auto projection{glm::ortho(0.0f, dims[0], dims[1], 0.0f, 0.0f, 1.0f)};
  const auto view{glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                              glm::vec3(0.0f, 1.0f, 0.0f))};

  globals::pv_ubo = pv_ubo::buffer::create();
  globals::pv_ubo.update_all(&projection, &view);

  // Create glyph cache with all settings we will use
  std::array<const char *, 3> fonts{"resources/DejaVuSans.ttf", "resources/DejaVuSansMono.ttf",
                                    "resources/DejaVuSerif.ttf"};

  std::array<text_database::glyph_cache::font_size_t, 3> sizes{40, 40, 40};

  std::array<text_database::glyph_cache::font_res_t, 3> resolutions{300, 300, 300};

  std::array langs{text_database::glyph_cache::languages::en_US,
                   text_database::glyph_cache::languages::en_US,
                   text_database::glyph_cache::languages::pt_BR};

  const text_database::glyph_cache::create_info txt_gc_ci{
      .fonts = fonts, .sizes_in_pts = sizes, .resolution_dpis = resolutions, .langs = langs};

  const auto txt_gc{text_database::glyph_cache::create(txt_gc_ci)};
  if (!txt_gc) {
    log_error("Unable to initialize sprite database");
    return static_cast<int>(txt_gc.error());
  } else {
    globals::txt_gc = *txt_gc;
  }

  text_database::glyph_cache::reside_all(globals::txt_gc);

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto on_unload() -> int {
  using namespace surge;

  log_info("Unloading Text Demo module");

  log_info("Waiting OpenGL idle");
  renderer::gl::wait_idle();

  gl_atom::text_database::glyph_cache::destroy(globals::txt_gc);
  globals::pv_ubo.destroy();

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto draw() -> int {
  globals::pv_ubo.bind_to_location(2);
  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto update(double) -> int { return 0; }

extern "C" SURGE_MODULE_EXPORT void keyboard_event(int, int, int, int) {}

extern "C" SURGE_MODULE_EXPORT void mouse_button_event(int, int, int) {}

extern "C" SURGE_MODULE_EXPORT void mouse_scroll_event(double, double) {}