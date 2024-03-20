// clang-format off
#include "DTU.hpp"

#include "player/logging.hpp"
#include "player/error_types.hpp"
#include "player/texture.hpp"
#include "player/window.hpp"
#include "mpsb.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#  include <tracy/TracyOpenGL.hpp>
#endif
// clang-format on

namespace shader_programs {

static GLuint sprite_shader{0}; // NOLINT
static GLuint text_shader{0};   // NOLINT

} // namespace shader_programs

namespace globals {

static surge::atom::texture::database tdb; // NOLINT
static GLuint MPSB{0};                     // NOLINT;

} // namespace globals

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
  globals::tdb = texture::database::create(32);

  // Initialize global 2D projection matrix and view matrix
  const auto [ww, wh] = surge::window::get_dims(window);
  const auto projection{glm::ortho(0.0f, ww, wh, 0.0f, -1.1f, 1.1f)};
  const auto view{glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                              glm::vec3(0.0f, 1.0f, 0.0f))};

  // MPSB
  globals::MPSB = surge::atom::mpsb::create_buffer();
  surge::atom::mpsb::send_buffer(globals::MPSB, &projection, &view);

  // Sprite Shader
  const auto sprite_shader{
      surge::renderer::create_shader_program("shaders/sprite.vert", "shaders/sprite.frag")};
  if (!sprite_shader) {
    return static_cast<int>(sprite_shader.error());
  }
  shader_programs::sprite_shader = *sprite_shader;

  // Text shader
  const auto text_shader{
      surge::renderer::create_shader_program("shaders/text.vert", "shaders/text.frag")};
  if (!text_shader) {
    return static_cast<int>(text_shader.error());
  }
  shader_programs::text_shader = *text_shader;

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto on_unload(GLFWwindow *window) noexcept -> int {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("DTU::on_unload");
#endif

  using namespace surge;
  using namespace surge::atom;
  using namespace surge::renderer;

  destroy_shader_program(shader_programs::text_shader);
  destroy_shader_program(shader_programs::sprite_shader);

  mpsb::destroy_buffer(globals::MPSB);
  globals::tdb.destroy();

  const auto unbind_callback_stat{DTU::unbind_callbacks(window)};
  if (unbind_callback_stat != 0) {
    return unbind_callback_stat;
  }

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto draw() noexcept -> int { return 0; }

extern "C" SURGE_MODULE_EXPORT auto update(GLFWwindow *, double) noexcept -> int { return 0; }

extern "C" SURGE_MODULE_EXPORT void keyboard_event(GLFWwindow *, int, int, int, int) noexcept {}

extern "C" SURGE_MODULE_EXPORT void mouse_button_event(GLFWwindow *, int, int, int) noexcept {}

extern "C" SURGE_MODULE_EXPORT void mouse_scroll_event(GLFWwindow *, double, double) noexcept {}