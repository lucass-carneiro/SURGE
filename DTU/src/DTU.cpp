#include "DTU.hpp"

#include "game_state.hpp"

// clang-format off
#include "player/renderer.hpp"
#include "player/static_image.hpp"
#include "player/logging.hpp"
#include "player/error_types.hpp"
#include "player/logging.hpp"
// clang-format on

#include "main_menu/main_menu.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>

// View / Projection

static glm::mat4 g_projection_matrix{1.0f};
static auto g_view_matrix{glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                                      glm::vec3(0.0f, 1.0f, 0.0f))};

auto DTU::get_projection() noexcept -> const glm::mat4 & { return g_projection_matrix; }
auto DTU::get_view() noexcept -> glm::mat4 & { return g_view_matrix; }

// Shaders
static GLuint g_img_shader{0};

auto DTU::get_img_shader() noexcept -> GLuint { return g_img_shader; }

// Image components
static surge::vector<surge::atom::static_image::one_buffer_data> g_static_img_buffer_data{};
static surge::vector<surge::atom::static_image::one_draw_data> g_static_image_draw{};

auto DTU::components::static_image_buffer::get() noexcept
    -> surge::vector<surge::atom::static_image::one_buffer_data> & {
  return g_static_img_buffer_data;
}

auto DTU::components::static_image_draw::get() noexcept
    -> surge::vector<surge::atom::static_image::one_draw_data> & {
  return g_static_image_draw;
}

void DTU::components::static_image_buffer::reset() noexcept {
  for (auto &buffer : g_static_img_buffer_data) {
    surge::atom::static_image::cleanup(buffer);
  }
  g_static_img_buffer_data.clear();
}

void DTU::components::static_image_draw::reset() noexcept { g_static_image_draw.clear(); }

// States
static DTU::game_state g_front_state{};
// static DTU::game_state g_back_state{0};
static DTU::game_state &g_current_state{g_front_state};

// TODO: TEMP
#include "player/nonuniform_tiles.hpp"
static GLuint g_temp_shader{0};
static surge::atom::nonuniform_tiles::buffer_data g_buffer_data{};
static surge::atom::nonuniform_tiles::draw_data g_draw_data{};

// On load
extern "C" SURGE_MODULE_EXPORT auto on_load(GLFWwindow *window) noexcept -> int {
  // Bind callbacks
  const auto bind_callback_stat{DTU::bind_callbacks(window)};
  if (bind_callback_stat != 0) {
    return bind_callback_stat;
  }

  // Initialize global 2D projection matrix
  const auto [ww, wh] = surge::window::get_dims(window);
  g_projection_matrix = glm::ortho(0.0f, ww, wh, 0.0f, -1.1f, 1.1f);

  // Load the static image shader
  const auto img_shader{
      surge::renderer::create_shader_program("shaders/image.vert", "shaders/image.frag")};
  if (!img_shader) {
    return static_cast<int>(surge::error::static_image_shader_creation);
  }
  g_img_shader = *img_shader;

  // TODO: TEMP
  const auto temp_shader{surge::renderer::create_shader_program("shaders/nonuniform_tiles.vert",
                                                                "shaders/nonuniform_tiles.frag")};
  if (!temp_shader) {
    return static_cast<int>(surge::error::static_image_shader_creation);
  }
  g_temp_shader = *temp_shader;
  g_buffer_data = *(surge::atom::nonuniform_tiles::create("temp"));
  g_draw_data.projection = g_projection_matrix;
  g_draw_data.view = g_view_matrix;
  g_draw_data.positions
      = surge::vector<glm::vec3>{glm::vec3{0.0f, 0.0f, 0.1f}, glm::vec3{100.0f, 100.0f, 0.1f},
                                 glm::vec3{250.0f, 250.0f, 0.1f}};
  g_draw_data.scales
      = surge::vector<glm::vec3>{glm::vec3{50.0f}, glm::vec3{25.0f}, glm::vec3{25.0f, 50.0f, 1.0f}};

  // Pre allocate memory for component lists
  constexpr const std::size_t base_component_list_size{8};
  g_static_img_buffer_data.reserve(base_component_list_size);
  g_static_image_draw.reserve(base_component_list_size);

  // Load first game state
  g_front_state = DTU::game_state{DTU::state::main_menu::state_load,
                                  DTU::state::main_menu::state_unload, DTU::state::main_menu::draw};

  const auto state_load_result{g_current_state.state_load()};
  if (state_load_result != 0) {
    return state_load_result;
  }

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto on_unload(GLFWwindow *window) noexcept -> int {
  const auto unbind_callback_stat{DTU::unbind_callbacks(window)};
  if (unbind_callback_stat != 0) {
    return unbind_callback_stat;
  }

  const auto state_unload_result{g_current_state.state_unload()};
  if (state_unload_result != 0) {
    return state_unload_result;
  }

  surge::renderer::cleanup_shader_program(g_img_shader);

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto draw() noexcept -> int {
  surge::atom::nonuniform_tiles::draw(g_temp_shader, g_buffer_data, g_draw_data);
  return g_current_state.state_draw();
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

extern "C" SURGE_MODULE_EXPORT auto update(double) noexcept -> int { return 0; }

extern "C" SURGE_MODULE_EXPORT void keyboard_event(GLFWwindow *, int, int, int, int) noexcept {}

extern "C" SURGE_MODULE_EXPORT void mouse_button_event(GLFWwindow *, int, int, int) noexcept {}

extern "C" SURGE_MODULE_EXPORT void mouse_scroll_event(GLFWwindow *, double, double) noexcept {}