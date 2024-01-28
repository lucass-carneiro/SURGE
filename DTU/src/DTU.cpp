#include "DTU.hpp"

#include "states.hpp"

// clang-format off
#include "player/renderer.hpp"
#include "player/error_types.hpp"
#include "player/container_types.hpp"
#include "player/logging.hpp"
#include "player/sprite.hpp"
// clang-format on

#include "main_menu/main_menu.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

// NOLINTNEXTLINE
static glm::mat4 g_projection{};

// NOLINTNEXTLINE
static glm::mat4 g_view{};

// NOLINTNEXTLINE
static GLuint g_sprite_shader{0};

// NOLINTNEXTLINE
static surge::atom::sprite::buffer_data g_sprite_buffer{};

// NOLINTNEXTLINE
static surge::vector<glm::mat4> g_sprite_models{};

// NOLINTNEXTLINE
static surge::vector<GLuint64> g_sprite_texture_handles{};

// NOLINTNEXTLINE
static surge::vector<float> g_sprite_alphas{};

// NOLINTNEXTLINE
static DTU::state_id g_current_state_id{DTU::state_id::main_menu};

// NOLINTNEXTLINE
static surge::queue<surge::u32> g_command_queue{};

extern "C" SURGE_MODULE_EXPORT auto on_load(GLFWwindow *window) noexcept -> int {
  // Bind callbacks
  const auto bind_callback_stat{DTU::bind_callbacks(window)};
  if (bind_callback_stat != 0) {
    return bind_callback_stat;
  }

  // Initialize global 2D projection matrix
  const auto [ww, wh] = surge::window::get_dims(window);
  g_projection = glm::ortho(0.0f, ww, wh, 0.0f, -1.1f, 1.1f);

  // Initial camera view
  g_view = glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                       glm::vec3(0.0f, 1.0f, 0.0f));

  // Load Shaders
  const auto sprite_shader{
      surge::renderer::create_shader_program("shaders/sprite.vert", "shaders/sprite.frag")};
  if (!sprite_shader) {
    return static_cast<int>(sprite_shader.error());
  }
  g_sprite_shader = *sprite_shader;
  g_sprite_buffer = surge::atom::sprite::create_buffers();
  g_sprite_models.reserve(16);
  g_sprite_texture_handles.reserve(16);
  g_sprite_alphas.reserve(16);

  return DTU::state::main_menu::load(g_command_queue, g_sprite_models, g_sprite_texture_handles,
                                     g_sprite_alphas, ww, wh);
}

extern "C" SURGE_MODULE_EXPORT auto on_unload(GLFWwindow *window) noexcept -> int {

  switch (g_current_state_id) {
  case DTU::state_id::main_menu:
    return DTU::state::main_menu::unload(g_command_queue, g_sprite_models,
                                         g_sprite_texture_handles);
  default:
    break;
  }

  const auto unbind_callback_stat{DTU::unbind_callbacks(window)};
  if (unbind_callback_stat != 0) {
    return unbind_callback_stat;
  }

  surge::atom::sprite::make_non_resident(g_sprite_texture_handles);
  surge::atom::sprite::destroy_buffers(g_sprite_buffer);
  surge::renderer::cleanup_shader_program(g_sprite_shader);

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto draw() noexcept -> int {
  surge::atom::sprite::draw(g_sprite_shader, g_sprite_buffer, g_projection, g_view, g_sprite_models,
                            g_sprite_texture_handles, g_sprite_alphas);
  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto update(double dt) noexcept -> int {
  switch (g_current_state_id) {
  case DTU::state_id::main_menu:
    return DTU::state::main_menu::update(g_command_queue, g_sprite_models, g_sprite_alphas, dt);
  default:
    break;
  }
}

extern "C" SURGE_MODULE_EXPORT void keyboard_event(GLFWwindow *, int key, int scancode, int action,
                                                   int mods) noexcept {
  switch (g_current_state_id) {
  case DTU::state_id::main_menu:
    return DTU::state::main_menu::keyboard_event(g_command_queue, key, scancode, action, mods);
  default:
    break;
  }
}

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