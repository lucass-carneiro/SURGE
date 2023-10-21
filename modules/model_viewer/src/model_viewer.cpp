#include "model_viewer.hpp"

#include "logging.hpp"
#include "renderer.hpp"
#include "static_mesh.hpp"
#include "window.hpp"

#include <GLFW/glfw3.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>
#include <glm/trigonometric.hpp>

static glm::mat4 projection{1.0f};
static glm::mat4 view{1.0f};
static glm::mat4 model{1.0f};

static surge::atom::static_mesh::one_buffer_data static_mesh_bd{};

static GLuint static_mesh_shader{0};

auto mod_model_viewer::bind_callbacks(GLFWwindow *window) noexcept -> std::uint32_t {
  log_info("Binding interaction callbacks");

  glfwSetKeyCallback(window, keyboard_event);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to bind keyboard event callback");
    return static_cast<std::uint32_t>(mod_model_viewer::error::keyboard_event_unbinding);
  }

  glfwSetMouseButtonCallback(window, mouse_button_event);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to bind mouse button event callback");
    return static_cast<std::uint32_t>(mod_model_viewer::error::mouse_button_event_unbinding);
  }

  glfwSetScrollCallback(window, mouse_scroll_event);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to bind mouse scroll event callback");
    return static_cast<std::uint32_t>(mod_model_viewer::error::mouse_scroll_event_unbinding);
  }

  return 0;
}

auto mod_model_viewer::unbind_callbacks(GLFWwindow *window) noexcept -> std::uint32_t {
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

extern "C" SURGE_MODULE_EXPORT auto on_load(GLFWwindow *window) noexcept -> std::uint32_t {
  using namespace mod_model_viewer;

  const auto bind_callback_stat{bind_callbacks(window)};
  if (bind_callback_stat != 0) {
    return bind_callback_stat;
  }

  log_info("Creating perspective matrix");
  const auto [ww, wh] = surge::window::get_dims(window);
  projection = glm::perspective(glm::radians(45.0f), ww / wh, 0.1f, 100.0f);
  view = glm::lookAt(glm::vec3(10.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                     glm::vec3(0.0f, 0.0f, 1.0f));

  log_info("Loading static mesh shader");
  const auto shader{surge::renderer::create_shader_program("shaders/static_mesh.vert",
                                                           "shaders/static_mesh.frag")};
  if (!shader) {
    return static_cast<std::uint32_t>(shader.error());
  }

  log_info("Creating static mesh");
  const auto model{surge::atom::static_mesh::load("model.obj")};
  if (!model) {
    return static_cast<std::uint32_t>(model.error());
  } else {
    static_mesh_shader = *shader;
    static_mesh_bd = *model;
  }

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto on_unload(GLFWwindow *window) noexcept -> std::uint32_t {
  using namespace mod_model_viewer;

  const auto unbind_callback_stat{unbind_callbacks(window)};
  if (unbind_callback_stat != 0) {
    return unbind_callback_stat;
  }

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto draw() noexcept -> std::uint32_t {
  using namespace surge::atom;

  const static_mesh::one_draw_data static_mesh_dd{projection, view, model,
                                                  glm::vec4{1.0f, 0.0f, 0.0f, 1.0f}};
  static_mesh::draw(static_mesh_shader, static_mesh_bd, static_mesh_dd);
  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto update(double dt) noexcept -> std::uint32_t {
  model = glm::rotate(model, glm::radians(15.0f * static_cast<float>(dt)),
                      glm::vec3{0.0f, 0.0f, 1.0f});
  return 0;
}

extern "C" SURGE_MODULE_EXPORT void keyboard_event(GLFWwindow *, int key, int, int action,
                                                   int) noexcept {
  if (key == GLFW_KEY_UP && action == GLFW_PRESS) {
    view = glm::translate(view, glm::vec3{1.0, 0.0, 0.0});
  } else if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) {
    view = glm::translate(view, glm::vec3{-1.0f, 0.0, 0.0});
  } else if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
    surge::renderer::enable(surge::renderer::capability::wireframe);
  } else if (key == GLFW_KEY_F2 && action == GLFW_PRESS) {
    surge::renderer::disable(surge::renderer::capability::wireframe);
  }
}

extern "C" SURGE_MODULE_EXPORT void mouse_button_event(GLFWwindow *, int, int, int) noexcept {}

extern "C" SURGE_MODULE_EXPORT void mouse_scroll_event(GLFWwindow *, double, double) noexcept {}