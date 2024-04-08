// clang-format off
#include "DTU.hpp"
#include "state_machine.hpp"

#include "player/logging.hpp"
#include "player/error_types.hpp"
#include "player/texture.hpp"
#include "player/window.hpp"
#include "player/sprite.hpp"
#include "player/text.hpp"
#include "player/pv_ubo.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <cmath>

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#  include <tracy/TracyOpenGL.hpp>
#endif
// clang-format on

namespace globals {

static GLuint sprite_shader{0}; // NOLINT
static GLuint text_shader{0};   // NOLINT

static surge::atom::texture::database tdb{}; // NOLINT
static surge::atom::pv_ubo::buffer pv_ubo{}; // NOLINT
static surge::atom::sprite::database sdb{};  // NOLINT

static surge::atom::text::text_engine ten{}; // NOLINT
static surge::atom::text::glyph_cache gc0{}; // NOLINT
static surge::atom::text::glyph_cache gc1{}; // NOLINT
static surge::atom::text::text_buffer txb{}; // NOLINT

static DTU::cmdq_t cmdq{}; // NOLINT

static DTU::state_machine stm{};

} // namespace globals

extern "C" SURGE_MODULE_EXPORT auto on_load(GLFWwindow *window) noexcept -> int {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("DTU::on_load");
#endif

  using namespace surge;
  using namespace surge::atom;

  // Bind callbacks
  const auto bind_callback_stat{DTU::bind_callbacks(window)};
  if (bind_callback_stat != 0) {
    return bind_callback_stat;
  }

  // Texture database
  globals::tdb = texture::database::create(128);

  // Sprite database
  globals::sdb = sprite::database::create(128);

  // Text Engine
  const auto ten_result{text::text_engine::create()};
  if (ten_result) {
    globals::ten = *ten_result;
  } else {
    log_error("Unable to create text engine");
    return static_cast<int>(ten_result.error());
  }

  auto load_face_result{
      globals::ten.load_face("resources/fonts/ITC_Benguiat_Bold.ttf", "benguiat_bold")};
  if (load_face_result) {
    log_error("Unable to load resources/fonts/ITC_Benguiat_Bold.ttf");
    return static_cast<int>(*load_face_result);
  }

  load_face_result
      = globals::ten.load_face("resources/fonts/ITC_Benguiat_Book.ttf", "benguiat_book");
  if (load_face_result.has_value()) {
    log_error("Unable to load resources/fonts/ITC_Benguiat_Book.ttf");
    return static_cast<int>(load_face_result.value());
  }

  // Glyph Caches
  auto gc_result{text::glyph_cache::create(globals::ten.get_faces()["benguiat_bold"])};
  if (!gc_result) {
    log_error("Unable to create glyph cache for benguiat_bold");
    return static_cast<int>(gc_result.error());
  }

  globals::gc0 = *gc_result;

  gc_result = text::glyph_cache::create(globals::ten.get_faces()["benguiat_book"]);
  if (!gc_result) {
    log_error("Unable to create glyph cache for benguiat_book");
    return static_cast<int>(gc_result.error());
  }

  globals::gc1 = *gc_result;

  globals::gc0.make_resident();
  globals::gc1.make_resident();

  // Text Buffer
  globals::txb = surge::atom::text::text_buffer::create(540);

  // Initialize global 2D projection matrix and view matrix
  const auto [ww, wh] = surge::window::get_dims(window);
  const auto projection{glm::ortho(0.0f, ww, wh, 0.0f, -1.1f, 1.1f)};
  const auto view{glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                              glm::vec3(0.0f, 1.0f, 0.0f))};

  // PV UBO
  globals::pv_ubo = pv_ubo::buffer::create();
  globals::pv_ubo.update_all(&projection, &view);

  // Sprite Shader
  const auto sprite_shader{
      surge::renderer::create_shader_program("shaders/sprite.vert", "shaders/sprite.frag")};
  if (!sprite_shader) {
    return static_cast<int>(sprite_shader.error());
  }
  globals::sprite_shader = *sprite_shader;

  // Text shader
  const auto text_shader{
      surge::renderer::create_shader_program("shaders/text.vert", "shaders/text.frag")};
  if (!text_shader) {
    return static_cast<int>(text_shader.error());
  }
  globals::text_shader = *text_shader;

  // First state
  globals::stm.push(DTU::state::main_menu);
  const auto transition_result{globals::stm.transition(globals::tdb)};

  if (transition_result) {
    log_error("Error loading first state");
    return static_cast<int>(transition_result.value());
  }

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto on_unload(GLFWwindow *window) noexcept -> int {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("DTU::on_unload");
#endif

  using namespace surge;
  using namespace surge::atom;
  using namespace surge::renderer;

  globals::stm.destroy(globals::tdb);

  destroy_shader_program(globals::text_shader);
  destroy_shader_program(globals::sprite_shader);

  globals::txb.destroy();
  globals::gc1.destroy();
  globals::gc0.destroy();
  globals::ten.destroy();

  globals::pv_ubo.destroy();
  globals::sdb.destroy();

  globals::tdb.destroy();

  const auto unbind_callback_stat{DTU::unbind_callbacks(window)};
  if (unbind_callback_stat != 0) {
    return unbind_callback_stat;
  }

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto draw() noexcept -> int {
  globals::pv_ubo.bind_to_location(2);
  globals::sdb.draw(globals::sprite_shader);
  globals::txb.draw(globals::text_shader, glm::vec4{1.0f});
  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto update(GLFWwindow *window, double dt) noexcept -> int {
  using std::abs;

  // Clear buffers
  globals::sdb.reset();
  globals::txb.reset();

  // TODO: Temporary text test
  globals::txb.push(glm::vec3{0.0f, 768.0 / 2.0, 0.5f}, glm::vec2{0.75f}, globals::gc0,
                    "Hello World!");

  // Update states
  const auto transition_result{globals::stm.transition(globals::tdb)};
  const auto update_result{globals::stm.update(window, dt, globals::tdb, globals::sdb)};

  if (transition_result) {
    log_error("Unable to transition states");
    return static_cast<int>(transition_result.value());
  }

  if (update_result) {
    log_error("Unable to update current state");
    return static_cast<int>(update_result.value());
  }

  return 0;
}

extern "C" SURGE_MODULE_EXPORT void keyboard_event(GLFWwindow *, int, int, int, int) noexcept {}

extern "C" SURGE_MODULE_EXPORT void mouse_button_event(GLFWwindow *, int, int, int) noexcept {}

extern "C" SURGE_MODULE_EXPORT void mouse_scroll_event(GLFWwindow *, double, double) noexcept {}

auto DTU::bind_callbacks(GLFWwindow *window) noexcept -> int {
  log_info("Binding interaction callbacks");

  glfwSetKeyCallback(window, keyboard_event);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to bind keyboard event callback");
    return static_cast<int>(surge::error::keyboard_event_unbinding);
  }

  glfwSetMouseButtonCallback(window, mouse_button_event);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to bind mouse button event callback");
    return static_cast<int>(surge::error::mouse_button_event_unbinding);
  }

  glfwSetScrollCallback(window, mouse_scroll_event);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to bind mouse scroll event callback");
    return static_cast<int>(surge::error::mouse_scroll_event_unbinding);
  }

  return 0;
}

auto DTU::unbind_callbacks(GLFWwindow *window) noexcept -> int {
  log_info("Unbinding interaction callbacks");

  glfwSetKeyCallback(window, nullptr);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to unbind keyboard event callback");
  }

  glfwSetMouseButtonCallback(window, nullptr);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to unbind mouse button event callback");
  }

  glfwSetScrollCallback(window, nullptr);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to unbind mouse scroll event callback");
  }

  return 0;
}