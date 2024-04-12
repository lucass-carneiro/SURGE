// clang-format off
#include "DTU.hpp"
#include "state_machine.hpp"

#ifdef SURGE_BUILD_TYPE_Debug
#include "debug_window.hpp"
#endif

#include "player/logging.hpp"
#include "player/error_types.hpp"
#include "player/texture.hpp"
#include "player/window.hpp"
#include "player/sprite.hpp"
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

static DTU::txd_t txd{};

static DTU::cmdq_t cmdq{}; // NOLINT

static DTU::state_machine stm{};

#ifdef SURGE_BUILD_TYPE_Debug
static std::tuple<float, float> wdims{}; // NOLINT
static bool show_debug_window{true};     // NOLINT
#endif

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
    globals::txd.ten = *ten_result;
  } else {
    log_error("Unable to create text engine");
    return static_cast<int>(ten_result.error());
  }

  auto load_face_result{
      globals::txd.ten.load_face("resources/fonts/DaveauRegular.otf", "daveau_regular")};
  if (load_face_result.has_value()) {
    log_error("Unable to load resources/fonts/DaveauRegular.otf");
    return static_cast<int>(*load_face_result);
  }

  load_face_result = globals::txd.ten.load_face("resources/fonts/DaveauLight.otf", "daveau_light");
  if (load_face_result.has_value()) {
    log_error("Unable to load resources/fonts/DaveauLight.otf");
    return static_cast<int>(load_face_result.value());
  }

  // Glyph Caches
  auto gc_result{text::glyph_cache::create(globals::txd.ten.get_faces()["daveau_regular"])};
  if (!gc_result) {
    log_error("Unable to create glyph cache for daveau_regular");
    return static_cast<int>(gc_result.error());
  }

  globals::txd.gc0 = *gc_result;

  gc_result = text::glyph_cache::create(globals::txd.ten.get_faces()["daveau_light"]);
  if (!gc_result) {
    log_error("Unable to create glyph cache for daveau_light");
    return static_cast<int>(gc_result.error());
  }

  globals::txd.gc1 = *gc_result;

  globals::txd.gc0.make_resident();
  globals::txd.gc1.make_resident();

  // Text Buffer
  globals::txd.txb = surge::atom::text::text_buffer::create(540);

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

  // Debug window
#ifdef SURGE_BUILD_TYPE_Debug
  DTU::debug_window::create(window);
#endif

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto on_unload(GLFWwindow *window) noexcept -> int {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("DTU::on_unload");
#endif

  using namespace surge;
  using namespace surge::atom;
  using namespace surge::renderer;

  // Debug window
#ifdef SURGE_BUILD_TYPE_Debug
  DTU::debug_window::destroy();
#endif

  globals::stm.destroy(globals::tdb);

  destroy_shader_program(globals::text_shader);
  destroy_shader_program(globals::sprite_shader);

  globals::txd.txb.destroy();
  globals::txd.gc1.destroy();
  globals::txd.gc0.destroy();
  globals::txd.ten.destroy();

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
  globals::txd.txb.draw(globals::text_shader, globals::txd.draw_color);

  // Debug UI pass
#ifdef SURGE_BUILD_TYPE_Debug
  DTU::debug_window::draw(globals::wdims, globals::show_debug_window);
#endif

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto update(GLFWwindow *window, double dt) noexcept -> int {
  using std::abs;

  // Clear buffers
  globals::sdb.reset();
  globals::txd.txb.reset();

  // Update states
  const auto transition_result{globals::stm.transition(globals::tdb)};
  const auto update_result{
      globals::stm.update(window, dt, globals::tdb, globals::sdb, globals::txd)};

  if (transition_result) {
    log_error("Unable to transition states");
    return static_cast<int>(transition_result.value());
  }

  if (update_result) {
    log_error("Unable to update current state");
    return static_cast<int>(update_result.value());
  }

#ifdef SURGE_BUILD_TYPE_Debug
  globals::wdims = surge::window::get_dims(window);
#endif

  return 0;
}

extern "C" SURGE_MODULE_EXPORT void keyboard_event(GLFWwindow *, int key, int, int action,
                                                   int) noexcept {
  if (key == GLFW_KEY_F6 && action == GLFW_RELEASE) {
    globals::show_debug_window = !globals::show_debug_window;
    log_info("%s debug window", globals::show_debug_window ? "Showing" : "Hiding");
  }
}

extern "C" SURGE_MODULE_EXPORT void mouse_button_event(GLFWwindow *window, int button, int action,
                                                       int mods) noexcept {
#ifdef SURGE_BUILD_TYPE_Debug
  ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
#endif
}

extern "C" SURGE_MODULE_EXPORT void mouse_scroll_event(GLFWwindow *window, double xoffset,
                                                       double yoffset) noexcept {
#ifdef SURGE_BUILD_TYPE_Debug
  ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
#endif
}

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