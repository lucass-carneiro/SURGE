#include "2048.hpp"

#include "logging.hpp"
#include "renderer.hpp"
#include "static_image.hpp"

static surge::atom::static_image::st_buffer_data<2> g_buffer_data{};

auto mod_2048::bind_callbacks(GLFWwindow *window) noexcept -> std::uint32_t {
  log_info("Binding interaction callbacks");

  glfwSetKeyCallback(window, keyboard_event);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to bind keyboard event callback");
    return static_cast<std::uint32_t>(mod_2048::error::keyboard_event_unbinding);
  }

  glfwSetMouseButtonCallback(window, mouse_button_event);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to bind mouse button event callback");
    return static_cast<std::uint32_t>(mod_2048::error::mouse_button_event_unbinding);
  }

  glfwSetScrollCallback(window, mouse_scroll_event);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to bind mouse scroll event callback");
    return static_cast<std::uint32_t>(mod_2048::error::mouse_scroll_event_unbinding);
  }

  return 0;
}

auto mod_2048::unbind_callbacks(GLFWwindow *window) noexcept -> std::uint32_t {
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

auto on_load(GLFWwindow *window) noexcept -> std::uint32_t {
  using namespace mod_2048;
  using namespace surge;
  using namespace surge::atom;

  // Bind callbacks
  const auto bind_callback_stat{bind_callbacks(window)};
  if (bind_callback_stat != 0) {
    return bind_callback_stat;
  }

  // Load the static image shader
  const auto img_shader{
      renderer::create_shader_program("shaders/image.vert", "shaders/image.frag")};
  if (!img_shader) {
    return static_cast<std::uint32_t>(error::img_shader_load);
  }

  // Load board image2
  const auto board_buffer{static_image::create("resources/board_normal.png")};
  if (!board_buffer) {
    return static_cast<std::uint32_t>(error::board_img_load);
  }

  // Load pieces image
  const auto pieces_buffer{static_image::create("resources/pieces.png")};
  if (!pieces_buffer) {
    return static_cast<std::uint32_t>(error::pieces_img_load);
  }

  return 0;
}

auto on_unload(GLFWwindow *window) noexcept -> std::uint32_t {
  using namespace mod_2048;

  const auto unbind_callback_stat{unbind_callbacks(window)};
  if (unbind_callback_stat != 0) {
    return unbind_callback_stat;
  }

  return 0;
}

auto draw() noexcept -> std::uint32_t { return 0; }
auto update(double dt) noexcept -> std::uint32_t { return 0; }
void keyboard_event(GLFWwindow *window, int key, int scancode, int action, int mods) noexcept {}
void mouse_button_event(GLFWwindow *window, int button, int action, int mods) noexcept {}
void mouse_scroll_event(GLFWwindow *window, double xoffset, double yoffset) noexcept {}