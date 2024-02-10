#include "DTU.hpp"

#include "debug_window.hpp"
#include "states.hpp"

// clang-format off
#include "player/renderer.hpp"
#include "player/error_types.hpp"
#include "player/container_types.hpp"
#include "player/logging.hpp"
#include "player/sprite.hpp"
#include "player/text.hpp"
#include "player/options.hpp"
// clang-format on

#include "main_menu/main_menu.hpp"
#include "new_game/new_game.hpp"

#include <cmath>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

// NOLINTNEXTLINE
static glm::mat4 g_projection{};

// NOLINTNEXTLINE
static glm::mat4 g_view{};

// NOLINTNEXTLINE
static GLuint g_sprite_shader{0};

// NOLINTNEXTLINE
static GLuint g_text_shader{0};

// NOLINTNEXTLINE
static surge::atom::sprite::buffer_data g_sprite_buffer{};

// NOLINTNEXTLINE
static surge::atom::sprite::data_list g_sprite_list_0{};

// NOLINTNEXTLINE
static surge::atom::text::glyph_data g_itc_benguiat_book_glyphs{};

// NOLINTNEXTLINE
static surge::atom::text::text_draw_data g_text_draw_buffer{};

// NOLINTNEXTLINE
static surge::deque<surge::u32> g_command_queue{};

// NOLINTNEXTLINE
static DTU::state_machine::state_t g_state_a{DTU::state_machine::states::no_state};

// NOLINTNEXTLINE
static DTU::state_machine::state_t g_state_b{DTU::state_machine::states::no_state};

void DTU::state_machine::push_state(state_t state) noexcept { g_state_b = state; }

static void unload_a() noexcept {
  switch (g_state_a) {

  case DTU::state_machine::states::main_menu:
    DTU::state::main_menu::unload(g_command_queue, g_sprite_list_0);
    break;

  case DTU::state_machine::states::new_game:
    DTU::state::new_game::unload(g_command_queue, g_sprite_list_0);
    break;

  default:
    break;
  }
}

static void load_a(float ww, float wh) noexcept {
  switch (g_state_a) {

  case DTU::state_machine::states::main_menu:
    DTU::state::main_menu::load(g_command_queue, g_sprite_list_0, ww, wh);
    break;

  case DTU::state_machine::states::new_game:
    DTU::state::new_game::load(g_command_queue, g_sprite_list_0, ww, wh);
    break;

  default:
    break;
  }
}

void DTU::state_machine::transition(float ww, float wh) noexcept {

  const auto a_empty_b_empty{g_state_a == states::no_state && g_state_b == states::no_state};
  const auto a_full_b_empty{g_state_a != states::no_state && g_state_b == states::no_state};
  const auto a_empty_b_full{g_state_a == states::no_state && g_state_b != states::no_state};
  const auto a_full_b_full{g_state_a != states::no_state && g_state_b != states::no_state};

  if (a_empty_b_empty || a_full_b_empty) {
  } else if (a_empty_b_full) {
    g_state_a = g_state_b;
    g_state_b = states::no_state;
    load_a(ww, wh);
  } else if (a_full_b_full) {
    unload_a();
    g_state_a = g_state_b;
    g_state_b = states::no_state;
    load_a(ww, wh);
  }
}

auto DTU::state_machine::get_a() noexcept -> state_t { return g_state_a; }
auto DTU::state_machine::get_b() noexcept -> state_t { return g_state_b; };

auto DTU::state_machine::to_str(state_t state) noexcept -> const char * {
  switch (state) {
  case states::no_state:
    return "no state";
  case states::exit_game:
    return "exit game";
  case states::new_game:
    return "new game";
  case states::main_menu:
    return "main menu";
  default:
    return "unknown state";
  }
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
  g_sprite_list_0.alphas.reserve(16);
  g_sprite_list_0.models.reserve(16);
  g_sprite_list_0.texture_handles.reserve(16);
  g_sprite_list_0.texture_ids.reserve(16);

  const auto text_shader{
      surge::renderer::create_shader_program("shaders/text.vert", "shaders/text.frag")};
  if (!text_shader) {
    return static_cast<int>(text_shader.error());
  }
  g_text_shader = *text_shader;

  // Load fonts used in the game and make their textures resident
  auto ft_lib{surge::atom::text::init_freetype()};
  if (!ft_lib) {
    return static_cast<int>(ft_lib.error());
  }

  auto itc_benguiat_book{
      surge::atom::text::load_face(*ft_lib, "resources/fonts/ITC_Benguiat_Book.ttf")};
  if (!itc_benguiat_book) {
    return static_cast<int>(itc_benguiat_book.error());
  }

  auto itc_benguiat_book_glyphs{surge::atom::text::load_glyphs(*ft_lib, *itc_benguiat_book, 32)};
  if (!itc_benguiat_book_glyphs) {
    return static_cast<int>(itc_benguiat_book_glyphs.error());
  }

  surge::atom::text::unload_face(*itc_benguiat_book);
  surge::atom::text::destroy_freetype(*ft_lib);

  g_itc_benguiat_book_glyphs = *itc_benguiat_book_glyphs;
  surge::atom::text::make_glyphs_resident(g_itc_benguiat_book_glyphs);

  // Allocate memory for the text buffers
  g_text_draw_buffer.texture_handles.reserve(32);
  g_text_draw_buffer.glyph_models.reserve(32);

  // First state
  DTU::state_machine::push_state(DTU::state_machine::states::main_menu);
  DTU::state_machine::transition(ww, wh);

  // Init debug window
#ifdef SURGE_BUILD_TYPE_Debug
  DTU::debug_window::init(window);
#endif

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto on_unload(GLFWwindow *window) noexcept -> int {
  unload_a();

  const auto unbind_callback_stat{DTU::unbind_callbacks(window)};
  if (unbind_callback_stat != 0) {
    return unbind_callback_stat;
  }

  surge::atom::sprite::make_non_resident(g_sprite_list_0.texture_handles);
  surge::atom::text::make_glyphs_non_resident(g_itc_benguiat_book_glyphs);

  surge::atom::text::unload_glyphs(g_itc_benguiat_book_glyphs);
  surge::atom::sprite::destroy_texture(g_sprite_list_0.texture_ids);
  surge::atom::sprite::destroy_buffers(g_sprite_buffer);

  surge::renderer::cleanup_shader_program(g_sprite_shader);
  surge::renderer::cleanup_shader_program(g_text_shader);

#ifdef SURGE_BUILD_TYPE_Debug
  DTU::debug_window::cleanup();
#endif

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto draw() noexcept -> int {
  surge::atom::sprite::draw(g_sprite_shader, g_sprite_buffer, g_projection, g_view,
                            g_sprite_list_0);

  surge::atom::text::draw(g_text_shader, g_sprite_buffer, g_projection, g_view, g_text_draw_buffer);

#ifdef SURGE_BUILD_TYPE_Debug
  DTU::debug_window::draw(g_command_queue, g_sprite_list_0);
#endif

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto update(double dt) noexcept -> int {
  using std::abs;

  // TODO: Update should take the window size. Getting it from the proj matrix is not reliable
  // TODO: State transitions should acomodate failures to load new state
  // Handle state transition
  const auto ww{2.0f / abs(g_projection[0][0])};
  const auto wh{2.0f / abs(g_projection[1][1])};
  DTU::state_machine::transition(ww, wh);

  // Update current state
  switch (g_state_a) {

  case DTU::state_machine::states::main_menu:
    DTU::state::main_menu::update(g_command_queue, g_sprite_list_0, dt);
    break;

  case DTU::state_machine::states::new_game:
    DTU::state::new_game::update(g_command_queue, g_sprite_list_0, dt);
    break;

  case DTU::state_machine::states::exit_game:
    return static_cast<int>(surge::error::normal_exit);

  default:
    break;
  }

  surge::atom::sprite::send_buffers(g_sprite_buffer, g_sprite_list_0);

  return 0;
}

extern "C" SURGE_MODULE_EXPORT void keyboard_event(GLFWwindow *, int key, int scancode, int action,
                                                   int mods) noexcept {
  switch (g_state_a) {

  case DTU::state_machine::states::main_menu:
    DTU::state::main_menu::keyboard_event(g_command_queue, key, scancode, action, mods);
    break;

  case DTU::state_machine::states::new_game:
    DTU::state::new_game::keyboard_event(g_command_queue, key, scancode, action, mods);
    break;

  default:
    break;
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