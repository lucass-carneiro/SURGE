#include "2048.hpp"

#include "logging.hpp"
#include "renderer.hpp"
#include "window.hpp"

// clang-format off
#include "pieces.hpp"
#include "debug_window.hpp"
// clang-format on

#include <EASTL/algorithm.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>

/*
 * Globals
 */

static glm::mat4 g_projection_matrix{1.0f};

static const auto g_view_matrix{glm::lookAt(
    glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f))};

static mod_2048::buffer_t g_board_buffer{};
static mod_2048::buffer_t g_pieces_buffer{};

static GLuint g_img_shader{};

static mod_2048::draw_t g_board_draw_data{};

// clang-format off
static const mod_2048::texture_origins_t g_piece_texture_origins{
  glm::vec2{1.0f  , 1.0f  },
  glm::vec2{104.0f, 1.0f  },
  glm::vec2{207.0f, 1.0f  },
  glm::vec2{310.0f, 1.0f  },
  glm::vec2{1.0f  , 104.0f},
  glm::vec2{104.0f, 104.0f},
  glm::vec2{207.0f, 104.0f},
  glm::vec2{310.0f, 104.0f},
  glm::vec2{1.0f  , 207.0f},
  glm::vec2{104.0f, 207.0f},
  glm::vec2{207.0f, 207.0f}
};
// clang-format on

// clang-format off
static const mod_2048::slot_coords_t g_slot_coords {
  glm::vec3{9.0f  , 9.0f  , 0.1f},
  glm::vec3{120.0f, 9.0f  , 0.1f},
  glm::vec3{231.0f, 9.0f  , 0.1f},
  glm::vec3{342.0f, 9.0f  , 0.1f},
  glm::vec3{9.0f  , 120.0f, 0.1f},
  glm::vec3{120.0f, 120.0f, 0.1f},
  glm::vec3{231.0f, 120.0f, 0.1f},
  glm::vec3{342.0f, 120.0f, 0.1f},
  glm::vec3{9.0f  , 231.0f, 0.1f},
  glm::vec3{120.0f, 231.0f, 0.1f},
  glm::vec3{231.0f, 231.0f, 0.1f},
  glm::vec3{342.0f, 231.0f, 0.1f},
  glm::vec3{9.0f  , 342.0f, 0.1f},
  glm::vec3{120.0f, 342.0f, 0.1f},
  glm::vec3{231.0f, 342.0f, 0.1f},
  glm::vec3{342.0f, 342.0f, 0.1f}
};
// clang-format on

static const float g_slot_size = 102.0f;

static const float g_slot_delta = g_slot_coords[1][0] - g_slot_coords[0][0];

static mod_2048::pieces::piece_id_queue_t g_piece_id_queue{};

static mod_2048::pieces::piece_positions_t g_piece_positions{};

static mod_2048::pieces::piece_exponents_t g_piece_exponents{};

static mod_2048::pieces::piece_slots_t g_piece_slots{};

/*
 * Global handlers and accessors
 */

auto mod_2048::get_projection() noexcept -> const glm::mat4 & { return g_projection_matrix; }

auto mod_2048::get_view() noexcept -> const glm::mat4 & { return g_view_matrix; }

auto mod_2048::get_board_buffer() noexcept -> const buffer_t & { return g_board_buffer; }

auto mod_2048::get_pieces_buffer() noexcept -> const buffer_t & { return g_pieces_buffer; }

auto mod_2048::get_img_shader() noexcept -> GLuint { return g_img_shader; }

auto mod_2048::get_board_draw_data() noexcept -> const draw_t & { return g_board_draw_data; }

auto mod_2048::get_piece_texture_origins() noexcept -> texture_origins_t & {
  return g_piece_texture_origins;
}

auto mod_2048::get_slot_coords() noexcept -> const slot_coords_t & { return g_slot_coords; }

auto mod_2048::get_slot_size() noexcept -> float { return g_slot_size; }

auto mod_2048::get_slot_delta() noexcept -> float { return g_slot_delta; }

auto mod_2048::pieces::get_piece_positions() noexcept -> const piece_positions_t & {
  return g_piece_positions;
}

auto mod_2048::pieces::get_piece_exponents() noexcept -> const piece_exponents_t & {
  return g_piece_exponents;
}

auto mod_2048::pieces::get_piece_slots() noexcept -> const piece_slots_t & { return g_piece_slots; }

auto mod_2048::pieces::create_piece(exponent_t exponent, slot_t slot) noexcept
    -> mod_2048::pieces::piece_id_t {

  // Gen ID
  const auto id{g_piece_id_queue.front()};
  g_piece_id_queue.pop_front();

  // Store components
  g_piece_positions[id] = g_slot_coords[slot];
  g_piece_exponents[id] = exponent;
  g_piece_slots[id] = slot;

  return id;
}

void mod_2048::pieces::delete_piece(piece_id_t piece_id) noexcept {
  if (eastl::find(g_piece_id_queue.begin(), g_piece_id_queue.end(), piece_id)
      != g_piece_id_queue.end()) {
    log_warn("Unable to remove piece id %u because it is already non existant", piece_id);
  } else {
    g_piece_positions.erase(piece_id);
    g_piece_exponents.erase(piece_id);
    g_piece_slots.erase(piece_id);
    g_piece_id_queue.push_back(piece_id);
  }
}

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

  // Initialize global 2D projection matrix
  const auto [ww, wh] = window::get_dims(window);
  g_projection_matrix = glm::ortho(0.0f, ww, wh, 0.0f, -1.1f, 1.1f);

  // Load the static image shader
  const auto img_shader{
      renderer::create_shader_program("shaders/image.vert", "shaders/image.frag")};
  if (!img_shader) {
    return static_cast<std::uint32_t>(error::img_shader_load);
  }
  g_img_shader = *img_shader;

  // Load board image2
  const auto board_buffer{static_image::create("resources/board_normal.png")};
  if (!board_buffer) {
    return static_cast<std::uint32_t>(error::board_img_load);
  }
  g_board_buffer = *board_buffer;

  // Load pieces image
  const auto pieces_buffer{static_image::create("resources/pieces.png")};
  if (!pieces_buffer) {
    return static_cast<std::uint32_t>(error::pieces_img_load);
  }
  g_pieces_buffer = *pieces_buffer;

  // Init board draw data
  g_board_draw_data.projection = g_projection_matrix;
  g_board_draw_data.view = g_view_matrix;
  g_board_draw_data.pos = glm::vec3{0.0};
  g_board_draw_data.scale = glm::vec3{ww, wh, 1.0f};
  g_board_draw_data.region_origin = glm::vec2{0.0f};
  g_board_draw_data.region_dims = glm::vec2{ww, wh};
  g_board_draw_data.h_flip = false;
  g_board_draw_data.v_flip = false;

  // Init piece ID queue
  for (pieces::piece_id_t i = 0; i < 16; i++) {
    g_piece_id_queue.push_back(i);
  }

  // Init debug window
  debug_window::init(window);

  pieces::create_piece(1, 0);

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

void mouse_button_event(GLFWwindow *window, int button, int action, int mods) noexcept {
  ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
}

void mouse_scroll_event(GLFWwindow *window, double xoffset, double yoffset) noexcept {
  ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
}

auto update(double) noexcept -> std::uint32_t { return 0; }
void keyboard_event(GLFWwindow *, int, int, int, int) noexcept {}