#include "DTU.hpp"

#include "states.hpp"

// clang-format off
#include "player/renderer.hpp"
#include "player/nonuniform_tiles.hpp"
#include "player/error_types.hpp"
#include "player/logging.hpp"
// clang-format on

#include "main_menu/main_menu.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

// NOLINTNEXTLINE
static glm::mat4 g_projection{};
static glm::mat4 g_view{};

// NOLINTNEXTLINE
static GLuint g_nonuniform_tile_shader{0};

// NOLINTNEXTLINE
static GLuint g_image_shader{0};

// NOLINTNEXTLINE
static DTU::state_id g_current_state_id{DTU::state_id::main_menu};

// NOLINTNEXTLINE
surge::queue<surge::u32> g_command_queue{};

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
  const auto nonuniform_tiles_shader{surge::renderer::create_shader_program(
      "shaders/nonuniform_tiles.vert", "shaders/nonuniform_tiles.frag")};
  if (!nonuniform_tiles_shader) {
    return static_cast<int>(surge::error::static_image_shader_creation);
  }
  g_nonuniform_tile_shader = *nonuniform_tiles_shader;

  const auto image_shader{
      surge::renderer::create_shader_program("shaders/image.vert", "shaders/image.frag")};
  if (!nonuniform_tiles_shader) {
    return static_cast<int>(surge::error::static_image_shader_creation);
  }
  g_image_shader = *image_shader;

  return DTU::state::main_menu::load(g_command_queue, ww, wh);
}

extern "C" SURGE_MODULE_EXPORT auto on_unload(GLFWwindow *window) noexcept -> int {
  const auto unbind_callback_stat{DTU::unbind_callbacks(window)};
  if (unbind_callback_stat != 0) {
    return unbind_callback_stat;
  }

  surge::renderer::cleanup_shader_program(g_nonuniform_tile_shader);
  surge::renderer::cleanup_shader_program(g_image_shader);

  switch (g_current_state_id) {
  case DTU::state_id::main_menu:
    return DTU::state::main_menu::unload(g_command_queue);
  default:
    break;
  }
}

extern "C" SURGE_MODULE_EXPORT auto draw() noexcept -> int {
  switch (g_current_state_id) {
  case DTU::state_id::main_menu:
    return DTU::state::main_menu::draw(
        DTU::state::main_menu::shader_indices{g_nonuniform_tile_shader, g_image_shader},
        g_projection, g_view);
  default:
    break;
  }
}

extern "C" SURGE_MODULE_EXPORT auto update(double dt) noexcept -> int {
  switch (g_current_state_id) {
  case DTU::state_id::main_menu:
    return DTU::state::main_menu::update(g_command_queue, dt);
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