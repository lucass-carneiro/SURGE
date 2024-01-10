#include "2048.hpp"
#include "logging.hpp"
#include "renderer.hpp"
#include "static_image.hpp"

#ifdef SURGE_BUILD_TYPE_Debug
#  include "debug_window.hpp"
#endif

// clang-format off
#include "pieces.hpp"
#include "text.hpp"
// clang-format on

#include <foonathan/memory/static_allocator.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#endif

/*
 * Globals
 */

static glm::mat4 g_projection_matrix{1.0f};

static const auto g_view_matrix{glm::lookAt(
    glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f))};

static mod_2048::buffer_t g_board_buffer{};
static mod_2048::buffer_t g_pieces_buffer{};

static mod_2048::text_buffer_t g_text_buffer{};
static mod_2048::text_charmap_t g_text_charmap{};

static GLuint g_img_shader{};
static GLuint g_txt_shader{};

static mod_2048::draw_t g_board_draw_data{};

// clang-format off
static const mod_2048::texture_origins_t g_piece_texture_origins{
  glm::vec2{1.0f  , 1.0f},
  glm::vec2{107.0f, 1.0f},
  glm::vec2{213.0f, 1.0f},
  glm::vec2{319.0f, 1.0f},

  glm::vec2{1.0f  , 107.0f},
  glm::vec2{107.0f, 107.0f},
  glm::vec2{213.0f, 107.0f},
  glm::vec2{319.0f, 107.0f},

  glm::vec2{1.0f  , 213.0f},
  glm::vec2{107.0f, 213.0f},
  glm::vec2{213.0f, 213.0f}
};
// clang-format on

// clang-format off
static const mod_2048::slot_coords_t g_slot_coords {
  glm::vec3{15.0f , 315.0f, 0.1f},
  glm::vec3{136.0f, 315.0f, 0.1f},
  glm::vec3{257.0f, 315.0f, 0.1f},
  glm::vec3{378.0f, 315.0f, 0.1f},

  glm::vec3{15.0f , 436.0f, 0.1f},
  glm::vec3{136.0f, 436.0f, 0.1f},
  glm::vec3{257.0f, 436.0f, 0.1f},
  glm::vec3{378.0f, 436.0f, 0.1f},

  glm::vec3{15.0f , 557.0f, 0.1f},
  glm::vec3{136.0f, 557.0f, 0.1f},
  glm::vec3{257.0f, 557.0f, 0.1f},
  glm::vec3{378.0f, 557.0f, 0.1f},

  glm::vec3{15.0f , 678.0f, 0.1f},
  glm::vec3{136.0f, 678.0f, 0.1f},
  glm::vec3{257.0f, 678.0f, 0.1f},
  glm::vec3{378.0f, 678.0f, 0.1f}
};
// clang-format on

static const float g_slot_size = 105.0f;

static const float g_slot_delta = g_slot_coords[1][0] - g_slot_coords[0][0];

static const glm::vec2 g_new_game_button_corner{358.0, 66.0};
static const glm::vec2 g_new_game_button_extent{138.0, 40.0};

static mod_2048::pieces::piece_id_queue_t g_piece_id_queue{};

static mod_2048::pieces::piece_id_queue_t g_stale_pieces_queue{};

static mod_2048::pieces::piece_positions_t g_piece_positions{};

static mod_2048::pieces::piece_exponents_t g_piece_exponents{};
static mod_2048::pieces::piece_exponents_t g_piece_target_exponents{};

static mod_2048::pieces::piece_slots_t g_piece_slots{};
static mod_2048::pieces::piece_slots_t g_piece_target_slots{};

static mod_2048::state_queue g_state_queue{};

static bool g_should_add_new_piece{false};

static mod_2048::points_t g_game_points{0};
static mod_2048::points_t g_best_score{0};

/*
 * Global handlers and accessors
 */

auto mod_2048::get_projection() noexcept -> const glm::mat4 & {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::get_projection()");
#endif

  return g_projection_matrix;
}

auto mod_2048::get_view() noexcept -> const glm::mat4 & {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::get_view()");
#endif

  return g_view_matrix;
}

auto mod_2048::get_board_buffer() noexcept -> const buffer_t & {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::get_board_buffer()");
#endif

  return g_board_buffer;
}

auto mod_2048::get_pieces_buffer() noexcept -> const buffer_t & {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::get_pieces_buffer()");
#endif

  return g_pieces_buffer;
}

auto mod_2048::get_text_buffer() noexcept -> const text_buffer_t & {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::get_text_buffer()");
#endif

  return g_text_buffer;
}

auto mod_2048::get_text_charmap() noexcept -> const text_charmap_t & {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::get_text_charmap()");
#endif

  return g_text_charmap;
}

auto mod_2048::get_img_shader() noexcept -> GLuint {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::get_img_shader()");
#endif

  return g_img_shader;
}

auto mod_2048::get_txt_shader() noexcept -> GLuint {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::get_txt_shader()");
#endif

  return g_txt_shader;
}

auto mod_2048::get_board_draw_data() noexcept -> const draw_t & {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::get_board_draw_data()");
#endif

  return g_board_draw_data;
}

auto mod_2048::get_piece_texture_origins() noexcept -> texture_origins_t & {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::get_piece_texture_origins()");
#endif

  return g_piece_texture_origins;
}

auto mod_2048::get_slot_coords() noexcept -> const slot_coords_t & {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::get_slot_coords()");
#endif

  return g_slot_coords;
}

auto mod_2048::get_slot_size() noexcept -> float {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::get_slot_size()");
#endif

  return g_slot_size;
}

auto mod_2048::get_slot_delta() noexcept -> float {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::get_slot_delta()");
#endif

  return g_slot_delta;
}

auto mod_2048::get_game_score() noexcept -> points_t & {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::get_game_score()");
#endif

  return g_game_points;
}
void mod_2048::add_game_score(points_t points) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::add_game_score()");
#endif

  g_game_points += points;
}
auto mod_2048::get_best_score() noexcept -> points_t & {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::get_best_score()");
#endif

  return g_best_score;
}

auto mod_2048::inside_new_game_button(double x, double y) noexcept -> bool {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::inside_new_game_button()");
#endif

  const auto in_x{g_new_game_button_corner[0] < x
                  && x < (g_new_game_button_corner[0] + g_new_game_button_extent[0])};
  const auto in_y{g_new_game_button_corner[1] < y
                  && y < (g_new_game_button_corner[1] + g_new_game_button_extent[1])};
  return in_x && in_y;
}

auto mod_2048::pieces::get_piece_positions() noexcept -> piece_positions_t & {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::get_piece_positions()");
#endif

  return g_piece_positions;
}

auto mod_2048::pieces::get_piece_exponents() noexcept -> piece_exponents_t & {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::get_piece_exponents()");
#endif

  return g_piece_exponents;
}

auto mod_2048::pieces::get_piece_target_exponents() noexcept -> piece_exponents_t & {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::get_piece_target_exponents()");
#endif

  return g_piece_target_exponents;
}

auto mod_2048::pieces::get_piece_slots() noexcept -> piece_slots_t & {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::get_piece_slots()");
#endif

  return g_piece_slots;
}

auto mod_2048::pieces::get_piece_target_slots() noexcept -> piece_slots_t & {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::get_piece_target_slots()");
#endif

  return g_piece_target_slots;
}

auto mod_2048::view_state_queue() noexcept -> const state_queue & {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::view_state_queue()");
#endif

  return g_state_queue;
}

auto mod_2048::get_state_queue() noexcept -> state_queue & {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::get_state_queue()");
#endif

  return g_state_queue;
}

auto mod_2048::pieces::create_piece(exponent_t exponent, slot_t slot) noexcept
    -> mod_2048::pieces::piece_id_t {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::pieces::create_piece");
#endif

  // Gen ID
  const auto id{g_piece_id_queue.front()};
  g_piece_id_queue.pop_front();

  // Store components
  g_piece_positions[id] = g_slot_coords[slot];

  g_piece_exponents[id] = exponent;
  g_piece_target_exponents[id] = exponent;

  g_piece_slots[id] = slot;
  g_piece_target_slots[id] = slot;

  return id;
}

void mod_2048::pieces::delete_piece(piece_id_t piece_id) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::pieces::delete_piece");
#endif

  if (std::find(g_piece_id_queue.begin(), g_piece_id_queue.end(), piece_id)
      != g_piece_id_queue.end()) {
    log_debug("Unable to remove piece id %u because it is already non existant", piece_id);
  } else {
    g_piece_positions.erase(piece_id);

    g_piece_exponents.erase(piece_id);
    g_piece_target_exponents.erase(piece_id);

    g_piece_slots.erase(piece_id);
    g_piece_target_slots.erase(piece_id);

    g_piece_id_queue.push_back(piece_id);
  }
}

void mod_2048::pieces::mark_stale(piece_id_t piece) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::pieces::mark_stale");
#endif

  g_stale_pieces_queue.push_back(piece);
}

void mod_2048::pieces::remove_stale() noexcept {
  log_debug("Removing stale pieces");

  while (g_stale_pieces_queue.size() != 0) {
    delete_piece(g_stale_pieces_queue.front());
    g_stale_pieces_queue.pop_front();
  }
}

void mod_2048::pieces::should_add_new_piece(bool v) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::pieces::should_add_new_piece");
#endif

  g_should_add_new_piece = v;
}

void mod_2048::new_game() noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::pieces::new_game");
#endif

  // Clear all state
  g_piece_positions.clear();
  g_piece_exponents.clear();
  g_piece_target_exponents.clear();
  g_piece_slots.clear();
  g_piece_target_slots.clear();
  g_piece_id_queue.clear();
  g_state_queue.clear();

  if (g_game_points > g_best_score) {
    g_best_score = g_game_points;
  }

  g_game_points = 0;
  log_debug("Best score %llu", g_best_score);

  // Reset piece ID queue
  for (pieces::piece_id_t i = 0; i < 16; i++) {
    g_piece_id_queue.push_back(i);
  }

  // Init state stack
  g_state_queue.push_back(game_state::idle);

  pieces::create_random();
  pieces::create_random();
}

auto on_load(GLFWwindow *window) noexcept -> std::uint32_t {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::on_load");
#endif

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

  // Load text rendering shader
  const auto text_shader{renderer::create_shader_program("shaders/text.vert", "shaders/text.frag")};
  if (!text_shader) {
    return static_cast<std::uint32_t>(error::img_shader_load);
  }
  g_txt_shader = *text_shader;

  // Load board image
  const auto board_buffer{static_image::create("resources/board.png")};
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

  // Load font cache
  text::font_name_vec_t fonts{"resources/clear-sans/ClearSans-Bold.ttf",
                              "resources/clear-sans/ClearSans-Regular.ttf",
                              "resources/clear-sans/ClearSans-Medium.ttf"};
  const auto text_buffer{text::create(fonts)};
  if (!text_buffer) {
    return static_cast<std::uint32_t>(error::fonts_load);
  }
  g_text_buffer = *text_buffer;

  const auto text_charmap{text::create_charmap(g_text_buffer, 25)};
  if (!text_charmap) {
    return static_cast<std::uint32_t>(error::charmap_create);
  }
  g_text_charmap = *text_charmap;

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

  // Init state stack
  g_state_queue.push_back(game_state::idle);

  // Reserve memory for hash maps
  g_piece_positions.reserve(16);

  g_piece_exponents.reserve(16);
  g_piece_target_exponents.reserve(16);

  g_piece_slots.reserve(16);
  g_piece_target_slots.reserve(16);

// Init debug window
#ifdef SURGE_BUILD_TYPE_Debug
  debug_window::init(window);
#endif

  pieces::create_random();
  pieces::create_random();

  return 0;
}

auto on_unload(GLFWwindow *window) noexcept -> std::uint32_t {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::on_unload");
#endif

  using namespace surge::atom;
  using namespace mod_2048;

  const auto unbind_callback_stat{unbind_callbacks(window)};
  if (unbind_callback_stat != 0) {
    return unbind_callback_stat;
  }

  text::terminate(g_text_buffer);

  surge::atom::static_image::cleanup(g_board_buffer);
  surge::atom::static_image::cleanup(g_pieces_buffer);
  surge::renderer::cleanup_shader_program(g_img_shader);
  surge::renderer::cleanup_shader_program(g_txt_shader);

#ifdef SURGE_BUILD_TYPE_Debug
  debug_window::cleanup();
#endif

  return 0;
}

auto update(double dt) noexcept -> std::uint32_t {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::update");
#endif

  using namespace mod_2048;

  switch (static_cast<state_code_t>(g_state_queue.front())) {

  case static_cast<state_code_t>(game_state::compress_right):
    if (pieces::idle()) {
      pieces::compress_right();
      g_state_queue.pop_front();
    }
    break;

  case static_cast<state_code_t>(game_state::merge_right):
    if (pieces::idle()) {
      pieces::merge_right();
      g_state_queue.pop_front();
    }
    break;

  case static_cast<state_code_t>(game_state::compress_left):
    if (pieces::idle()) {
      pieces::compress_left();
      g_state_queue.pop_front();
    }
    break;

  case static_cast<state_code_t>(game_state::merge_left):
    if (pieces::idle()) {
      pieces::merge_left();
      g_state_queue.pop_front();
    }
    break;

  case static_cast<state_code_t>(game_state::compress_up):
    if (pieces::idle()) {
      pieces::compress_up();
      g_state_queue.pop_front();
    }
    break;

  case static_cast<state_code_t>(game_state::merge_up):
    if (pieces::idle()) {
      pieces::merge_up();
      g_state_queue.pop_front();
    }
    break;

  case static_cast<state_code_t>(game_state::compress_down):
    if (pieces::idle()) {
      pieces::compress_down();
      g_state_queue.pop_front();
    }
    break;

  case static_cast<state_code_t>(game_state::merge_down):
    if (pieces::idle()) {
      pieces::merge_down();
      g_state_queue.pop_front();
    }
    break;

  case static_cast<state_code_t>(game_state::piece_removal):
    if (pieces::idle()) {
      pieces::remove_stale();
      pieces::update_exponents();
      log_debug("Update game points: %llu", get_game_score());
      g_state_queue.pop_front();
    }
    break;

  case static_cast<state_code_t>(game_state::add_piece):
    if (pieces::idle()) {
      if (g_should_add_new_piece) {
        pieces::create_random();
        g_should_add_new_piece = false;
      }
      g_state_queue.pop_front();
    }
    break;

  default:
    return 0;
  }

  pieces::update_positions(dt);

  return 0;
}

void keyboard_event(GLFWwindow *, int key, int, int action, int) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::keyboard_event");
#endif

  using namespace mod_2048;

  // Examine state stack. Only push a move if the board is idle
  if (g_state_queue.front() == game_state::idle) {
    if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
      g_state_queue.pop_front();
      g_state_queue.push_back(game_state::compress_right);
      g_state_queue.push_back(game_state::merge_right);
      g_state_queue.push_back(game_state::piece_removal);
      g_state_queue.push_back(game_state::add_piece);
      g_state_queue.push_back(game_state::idle);

    } else if (key == GLFW_KEY_LEFT && action == GLFW_PRESS) {
      g_state_queue.pop_front();
      g_state_queue.push_back(game_state::compress_left);
      g_state_queue.push_back(game_state::merge_left);
      g_state_queue.push_back(game_state::piece_removal);
      g_state_queue.push_back(game_state::add_piece);
      g_state_queue.push_back(game_state::idle);

    } else if (key == GLFW_KEY_UP && action == GLFW_PRESS) {
      g_state_queue.pop_front();
      g_state_queue.push_back(game_state::compress_up);
      g_state_queue.push_back(game_state::merge_up);
      g_state_queue.push_back(game_state::piece_removal);
      g_state_queue.push_back(game_state::add_piece);
      g_state_queue.push_back(game_state::idle);

    } else if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) {
      g_state_queue.pop_front();
      g_state_queue.push_back(game_state::compress_down);
      g_state_queue.push_back(game_state::merge_down);
      g_state_queue.push_back(game_state::piece_removal);
      g_state_queue.push_back(game_state::add_piece);
      g_state_queue.push_back(game_state::idle);
    }
  }
}