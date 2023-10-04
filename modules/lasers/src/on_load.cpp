#include "allocators.hpp"
#include "lasers.hpp"

#include <EASTL/vector.h>
#include <cstddef>
#include <gsl/gsl-lite.hpp>

// NOLINTNEXTLINE
static glm::mat4 global_projection{1.0f};

static const glm::mat4 global_view{glm::lookAt(
    glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f))};

// NOLINTNEXTLINE
static GLuint global_smo_shader{0};

// NOLINTNEXTLINE
static surge::renderer::smo::context global_cell_grid_ctx{};

auto surge::mod::lasers::get_global_projection() noexcept -> const glm::mat4 & {
  return global_projection;
}

auto surge::mod::lasers::get_global_view() noexcept -> const glm::mat4 & { return global_view; }

auto surge::mod::lasers::get_global_smo_shader() noexcept -> const GLuint & {
  return global_smo_shader;
}

auto surge::mod::lasers::get_global_cell_grid_ctx() noexcept
    -> const surge::renderer::smo::context & {
  return global_cell_grid_ctx;
}

auto surge::mod::lasers::bind_callbacks(GLFWwindow *window) noexcept -> std::uint32_t {
  using namespace surge::mod::lasers;

  log_info("Binding interaction callbacks");

  glfwSetKeyCallback(window, keyboard_event);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to bind keyboard event callback");
    return static_cast<std::uint32_t>(error::keyboard_event_unbinding);
  }

  glfwSetMouseButtonCallback(window, mouse_button_event);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to bind mouse button event callback");
    return static_cast<std::uint32_t>(error::mouse_button_event_unbinding);
  }

  glfwSetScrollCallback(window, mouse_scroll_event);
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_warn("Unable to bind mouse scroll event callback");
    return static_cast<std::uint32_t>(error::mouse_scroll_event_unbinding);
  }

  return 0;
}

using vertex_data_t = eastl::vector<float, surge::allocators::eastl::gp_allocator>;

auto create_line_grid_data(float ww, float wh, std::size_t rows, std::size_t cols) noexcept
    -> vertex_data_t {
  const float dx{ww / gsl::narrow_cast<float>(cols)};
  const float dy{wh / gsl::narrow_cast<float>(rows)};

  const std::size_t data_size{6 * (rows - 1) + 6 * (cols - 1)};

  vertex_data_t vertex_data(data_size);
  vertex_data.reserve(data_size);

  std::size_t I = 0;

  for (std::size_t i = 1; i < rows; i++) {
    const auto yi{gsl::narrow_cast<float>(i) * dy};
    vertex_data[I] = 0.0f;
    vertex_data[I + 1] = yi;
    vertex_data[I + 2] = 0.1f;
    vertex_data[I + 3] = ww;
    vertex_data[I + 4] = yi;
    vertex_data[I + 5] = 0.1f;
    I += 6;
  }

  for (std::size_t i = 1; i < cols; i++) {
    const auto xi{gsl::narrow_cast<float>(i) * dx};
    vertex_data[I] = xi;
    vertex_data[I + 1] = 0.0f;
    vertex_data[I + 2] = 0.1f;
    vertex_data[I + 3] = xi;
    vertex_data[I + 4] = wh;
    vertex_data[I + 5] = 0.1f;
    I += 6;
  }

  return vertex_data;
}

extern "C" SURGE_MODULE_EXPORT auto on_load(GLFWwindow *window) noexcept -> std::uint32_t {
  using namespace surge;
  using namespace surge::mod::lasers;

  const auto bind_callback_stat{bind_callbacks(window)};
  if (bind_callback_stat != 0) {
    return bind_callback_stat;
  }

  log_info("Setting projection matrix");
  const auto [ww, wh] = window::get_dims(window);
  global_projection = glm::ortho(0.0f, ww, wh, 0.0f);

  log_info("Creating smo shader");
  const auto smo_shader{renderer::create_shader_program("shaders/smo.vert", "shaders/smo.frag")};
  if (!smo_shader) {
    log_error("Unable to create smo shader");
    return static_cast<std::uint32_t>(renderer::smo::error::shader_creation);
  } else {
    global_smo_shader = *smo_shader;
  }

  log_info("Creating line grid");
  {
    const auto vertex_data{create_line_grid_data(ww, wh, 8, 8)};

    global_cell_grid_ctx = renderer::smo::create(
        global_smo_shader, renderer::smo::vertex_data{vertex_data.data(), vertex_data.size()});
  }

  return 0;
}