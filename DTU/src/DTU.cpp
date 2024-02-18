#include "DTU.hpp"

#include "debug_window.hpp"
#include "states.hpp"

// clang-format off
#include "player/renderer.hpp"
#include "player/error_types.hpp"
#include "player/container_types.hpp"
#include "player/logging.hpp"
#include "player/sprite.hpp"
#include "player/mpsb.hpp"
#include "player/text.hpp"
#include "player/options.hpp"
#include "player/files.hpp"
// clang-format on

#include "main_menu/main_menu.hpp"
#include "new_game/new_game.hpp"

#include <cmath>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace shader_programs {

static GLuint sprite_shader{0}; // NOLINT
static GLuint text_shader{0};   // NOLINT

} // namespace shader_programs

namespace atom_buffers {

static GLuint MPSB{0};                                   // NOLINT
static surge::atom::sprite::buffer_data sprite_buffer{}; // NOLINT
static surge::atom::text::buffer_data text_buffer{};     // NOLINT

} // namespace atom_buffers

namespace loaded_data {

static DTU::vec_glui loaded_texture_IDs{};       // NOLINT
static DTU::vec_glui64 loaded_texture_handles{}; // NOLINT

static surge::atom::text::glyph_data itc_benguiat_book_glyphs{}; // NOLINT

} // namespace loaded_data

namespace draw_buffers {

static DTU::sdl_t sprite{}; // NOLINT

static surge::atom::text::text_draw_data text{}; // NOLINT

} // namespace draw_buffers

static surge::deque<surge::u32> g_command_queue{}; // NOLINT

// NOLINTNEXTLINE
static DTU::state_machine::state_t g_state_a{DTU::state_machine::states::no_state};

// NOLINTNEXTLINE
static DTU::state_machine::state_t g_state_b{DTU::state_machine::states::no_state};

auto DTU::load_texture(vec_glui &ids, vec_glui64 &handles, const char *img_path) noexcept
    -> GLuint64 {
  using namespace surge;

  auto img{files::load_image(img_path)};

  if (img) {
    const auto texture_data{
        atom::sprite::create_texture(*img, renderer::texture_filtering::nearest)};

    if (texture_data) {
      const auto [id, handle] = *texture_data;
      ids.push_back(id);
      handles.push_back(handle);
      files::free_image(*img);
      return handle;
    } else {
      ids.push_back(0);
      handles.push_back(0);
      files::free_image(*img);
      return 0;
    }
  } else {
    return 0;
  }
}

void DTU::unload_textures(vec_glui &ids, vec_glui64 &handles) noexcept {
  surge::atom::sprite::make_non_resident(handles);
  surge::atom::sprite::destroy_texture(ids);

  ids.clear();
  handles.clear();
}

void DTU::push_sprite(sdl_t &sdl, GLuint64 handle, glm::mat4 &&model, float alpha) noexcept {
  sdl.texture_handles.push_back(handle);
  sdl.models.push_back(model);
  sdl.alphas.push_back(alpha);
}

void DTU::clear_sprites(sdl_t &sdl) noexcept {
  sdl.texture_handles.clear();
  sdl.models.clear();
  sdl.alphas.clear();
}

void DTU::load_push_sprite(vec_glui &ids, vec_glui64 &handles, const char *img_path, sdl_t &sdl,
                           glm::mat4 &&model, float alpha) noexcept {
  const auto handle{load_texture(ids, handles, img_path)};
  push_sprite(sdl, handle, std::move(model), alpha);
}

auto DTU::make_model(glm::vec3 &&pos, glm::vec3 &&scale) noexcept -> glm::mat4 {
  return glm::scale(glm::translate(glm::mat4{1.0f}, pos), scale);
}

void DTU::state_machine::push_state(state_t state) noexcept { g_state_b = state; }

static void unload_a() noexcept {
  switch (g_state_a) {

  case DTU::state_machine::states::main_menu:
    DTU::state::main_menu::unload(g_command_queue, loaded_data::loaded_texture_IDs,
                                  loaded_data::loaded_texture_handles, draw_buffers::sprite);
    break;

  case DTU::state_machine::states::new_game:
    DTU::state::new_game::unload(g_command_queue, draw_buffers::sprite);
    break;

  default:
    break;
  }
}

static void load_a(float ww, float wh) noexcept {
  switch (g_state_a) {

  case DTU::state_machine::states::main_menu:
    DTU::state::main_menu::load(g_command_queue, loaded_data::loaded_texture_IDs,
                                loaded_data::loaded_texture_handles, draw_buffers::sprite, ww, wh);
    break;

  case DTU::state_machine::states::new_game:
    DTU::state::new_game::load(g_command_queue, loaded_data::loaded_texture_IDs,
                               loaded_data::loaded_texture_handles, draw_buffers::sprite, ww, wh);
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
  // Limits
  constexpr const surge::usize max_sprites{32};
  constexpr const surge::usize max_chars_in_text{140};

  // Bind callbacks
  const auto bind_callback_stat{DTU::bind_callbacks(window)};
  if (bind_callback_stat != 0) {
    return bind_callback_stat;
  }

  // Initialize global 2D projection matrix and view matrix
  const auto [ww, wh] = surge::window::get_dims(window);
  const auto projection{glm::ortho(0.0f, ww, wh, 0.0f, -1.1f, 1.1f)};
  const auto view{glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                              glm::vec3(0.0f, 1.0f, 0.0f))};

  atom_buffers::MPSB = surge::atom::mpsb::create_buffer();
  surge::atom::mpsb::send_buffer(atom_buffers::MPSB, &projection, &view);

  // Sprite Shader
  const auto sprite_shader{
      surge::renderer::create_shader_program("shaders/sprite.vert", "shaders/sprite.frag")};
  if (!sprite_shader) {
    return static_cast<int>(sprite_shader.error());
  }
  shader_programs::sprite_shader = *sprite_shader;
  atom_buffers::sprite_buffer = surge::atom::sprite::create_buffers(max_sprites);

  // Text shader
  const auto text_shader{
      surge::renderer::create_shader_program("shaders/text.vert", "shaders/text.frag")};
  if (!text_shader) {
    return static_cast<int>(text_shader.error());
  }
  shader_programs::text_shader = *text_shader;
  atom_buffers::text_buffer = surge::atom::text::create_buffers(max_chars_in_text);

  // Load fonts
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

  loaded_data::itc_benguiat_book_glyphs = *itc_benguiat_book_glyphs;
  surge::atom::text::make_glyphs_resident(loaded_data::itc_benguiat_book_glyphs);

  // Allocate memory for loaded sprite textures
  loaded_data::loaded_texture_IDs.reserve(max_sprites);
  loaded_data::loaded_texture_handles.reserve(max_sprites);

  // Allocate memory for sprite draw buffers
  draw_buffers::sprite.alphas.reserve(max_sprites);
  draw_buffers::sprite.models.reserve(max_sprites);
  draw_buffers::sprite.texture_handles.reserve(max_sprites);

  // Allocate memory for text draw buffer
  draw_buffers::text.texture_handles.reserve(max_chars_in_text);
  draw_buffers::text.glyph_models.reserve(max_chars_in_text);
  draw_buffers::text.color = glm::vec4{1.0f, 1.0f, 1.0f, 1.0f};

  // First state
  // DTU::state_machine::push_state(DTU::state_machine::states::new_game);
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

  surge::atom::sprite::make_non_resident(loaded_data::loaded_texture_handles);
  surge::atom::sprite::destroy_texture(loaded_data::loaded_texture_IDs);

  surge::atom::text::make_glyphs_non_resident(loaded_data::itc_benguiat_book_glyphs);
  surge::atom::text::unload_glyphs(loaded_data::itc_benguiat_book_glyphs);

  surge::atom::text::destroy_buffers(atom_buffers::text_buffer);
  surge::atom::sprite::destroy_buffers(atom_buffers::sprite_buffer);
  surge::atom::mpsb::destroy_buffer(atom_buffers::MPSB);

  surge::renderer::destroy_shader_program(shader_programs::sprite_shader);
  surge::renderer::destroy_shader_program(shader_programs::text_shader);

#ifdef SURGE_BUILD_TYPE_Debug
  DTU::debug_window::cleanup();
#endif

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto draw() noexcept -> int {
  surge::atom::sprite::draw(shader_programs::sprite_shader, atom_buffers::sprite_buffer,
                            atom_buffers::MPSB, draw_buffers::sprite);

  surge::atom::text::draw(shader_programs::text_shader, atom_buffers::text_buffer,
                          atom_buffers::MPSB, draw_buffers::text);

#ifdef SURGE_BUILD_TYPE_Debug
  DTU::debug_window::draw(loaded_data::loaded_texture_IDs, loaded_data::loaded_texture_handles,
                          g_command_queue, draw_buffers::sprite);
#endif

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto update(GLFWwindow *window, double dt) noexcept -> int {
  using std::abs;

  // TODO: State transitions should acomodate failures to load new state
  // Handle state transition
  const auto [ww, wh] = surge::window::get_dims(window);
  DTU::state_machine::transition(ww, wh);

  // Update current state
  switch (g_state_a) {

  case DTU::state_machine::states::main_menu:
    DTU::state::main_menu::update(g_command_queue, draw_buffers::sprite, dt);
    break;

  case DTU::state_machine::states::new_game:
    DTU::state::new_game::update(window, g_command_queue, draw_buffers::sprite, draw_buffers::text,
                                 loaded_data::itc_benguiat_book_glyphs, dt);
    break;

  case DTU::state_machine::states::exit_game:
    return static_cast<int>(surge::error::normal_exit);

  default:
    break;
  }

  surge::atom::sprite::send_buffers(atom_buffers::sprite_buffer, draw_buffers::sprite);
  surge::atom::text::send_buffers(atom_buffers::text_buffer, draw_buffers::text);

  return 0;
}

extern "C" SURGE_MODULE_EXPORT void keyboard_event(GLFWwindow *, int key, int scancode, int action,
                                                   int mods) noexcept {
  switch (g_state_a) {

  case DTU::state_machine::states::main_menu:
    DTU::state::main_menu::keyboard_event(g_command_queue, key, scancode, action, mods);
    break;

  default:
    break;
  }
}

extern "C" SURGE_MODULE_EXPORT void mouse_button_event(GLFWwindow *window, int button, int action,
                                                       int mods) noexcept {
  switch (g_state_a) {

  case DTU::state_machine::states::new_game:
    DTU::state::new_game::mouse_click(g_command_queue, window, button, action, mods);
    break;

  default:
    break;
  }

#ifdef SURGE_BUILD_TYPE_Debug
  ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
#endif
}

extern "C" SURGE_MODULE_EXPORT void mouse_scroll_event(GLFWwindow *window, double xoffset,
                                                       double yoffset) noexcept {
  switch (g_state_a) {

  case DTU::state_machine::states::new_game:
    DTU::state::new_game::mouse_scroll(g_command_queue, window, xoffset, yoffset);
    break;

  default:
    break;
  }

#ifdef SURGE_BUILD_TYPE_Debug
  ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
#endif
}