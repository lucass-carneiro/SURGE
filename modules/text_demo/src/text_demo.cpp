#include "text_demo.hpp"

#include "sc_glm_includes.hpp"
#include "sc_logging.hpp"
#include "sc_opengl/atoms/pv_ubo.hpp"
#include "sc_opengl/atoms/text.hpp"
#include "sc_opengl/sc_opengl.hpp"
#include "sc_window.hpp"

#include <algorithm>
#include <array>
#include <cstdio>

using pv_ubo_t = surge::gl_atom::pv_ubo::buffer;

using txte_t = surge::gl_atom::text::text_engine;
using txtb_t = surge::gl_atom::text::text_buffer;
using txtgc_t = surge::gl_atom::text::glyph_cache;

namespace globals {

static txte_t txt_e{};
static txtb_t txt_b{};
static txtgc_t txt_gc{};

static pv_ubo_t pv_ubo{};

} // namespace globals

extern "C" SURGE_MODULE_EXPORT auto on_load(surge::window::window_t w) -> int {
  using namespace surge;
  using namespace gl_atom;

  log_info("Loading Text Demo module");

  // Text Engine
  const auto ten_result{gl_atom::text::text_engine::create()};
  if (ten_result) {
    globals::txt_e = *ten_result;
  } else {
    log_error("Unable to create text engine");
    return static_cast<int>(ten_result.error());
  }

  const auto load_face_result{globals::txt_e.load_face("resources/DejaVuSans.ttf", "font_a")};
  if (load_face_result.has_value()) {
    log_error("Unable to load resources/DejaVuSans.ttf");
    return static_cast<int>(*load_face_result);
  }

  // Glyph Caches
  const auto face{globals::txt_e.get_face("font_a")};
  if (!face) {
    log_error("Font font_a not found in cache");
    return static_cast<int>(error::freetype_null_face);
  }

  const auto glyph_cache{gl_atom::text::glyph_cache::create(*face)};
  if (!glyph_cache) {
    log_error("Unable to create glyph cache for dejavu_sans_bold");
    return static_cast<int>(glyph_cache.error());
  }

  globals::txt_gc = *glyph_cache;
  globals::txt_gc.make_resident();

  // Text Buffer
  const auto text_buffer{gl_atom::text::text_buffer::create(540)};
  if (!text_buffer) {
    log_error("Unable to create text buffer");
    return static_cast<int>(text_buffer.error());
  }
  globals::txt_b = *text_buffer;

  // PV UBO
  log_info("Creating projection and view matrices");
  const auto dims{window::get_dims(w)};
  const auto projection{glm::ortho(0.0f, dims[0], dims[1], 0.0f, 0.0f, 1.0f)};
  const auto view{glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                              glm::vec3(0.0f, 1.0f, 0.0f))};

  globals::pv_ubo = pv_ubo::buffer::create();
  globals::pv_ubo.update_all(&projection, &view);

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto on_unload(surge::window::window_t) -> int {
  using namespace surge;

  log_info("Unloading Text Demo module");

  log_info("Waiting OpenGL idle");
  renderer::gl::wait_idle();

  globals::txt_b.destroy();
  globals::txt_gc.destroy();
  globals::txt_e.destroy();

  globals::pv_ubo.destroy();

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto draw(surge::window::window_t) -> int {
  globals::pv_ubo.bind_to_location(2);
  globals::txt_b.draw(glm::vec4{1.0f});
  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto update(surge::window::window_t, double dt) -> int {
  using std::snprintf;

  auto &txt_gc{globals::txt_gc};
  auto &txt_b{globals::txt_b};

  txt_b.reset();

  txt_b.push(glm::vec3{10.0f, 40.0f, 0.1f}, glm::vec2{0.20f}, txt_gc,
             "This is SURGE's text rendering engine at work");

  txt_b.push(glm::vec3{10.0f, 80.0f, 0.1f}, glm::vec2{0.20f}, txt_gc,
             "The quick brown fox jumps over the lazy dog.");

  txt_b.push(glm::vec3{10.0f, 160.0f, 0.1f}, glm::vec2{0.20f}, txt_gc, "Current frame rate:");

  std::array<char, 8> fps_buffer{};
  std::fill(fps_buffer.begin(), fps_buffer.end(), '\0');
  snprintf(fps_buffer.data(), fps_buffer.size(), "%.0f", (1.0f / dt));

  txt_b.push(glm::vec3{350.0f, 160.0f, 0.1f}, glm::vec2{0.20f}, txt_gc, fps_buffer.data());

  return 0;
}

extern "C" SURGE_MODULE_EXPORT void keyboard_event(surge::window::window_t, int, int, int, int) {}

extern "C" SURGE_MODULE_EXPORT void mouse_button_event(surge::window::window_t, int, int, int) {}

extern "C" SURGE_MODULE_EXPORT void mouse_scroll_event(surge::window::window_t, double, double) {}