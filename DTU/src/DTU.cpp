#include "DTU.hpp"

#include "states.hpp"

// clang-format off
#include "player/renderer.hpp"
#include "player/error_types.hpp"
#include "player/logging.hpp"
#include "player/text.hpp"
#include "player/sprite.hpp"
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
static GLuint g_text_shader{0};

// NOLINTNEXTLINE
static GLuint g_sprite_shader{0};

// NOLINTNEXTLINE
static surge::atom::text::buffer_data g_text_buffer{};

// NOLINTNEXTLINE
static surge::atom::text::charmap_data g_text_charmap{};

// NOLINTNEXTLINE
static surge::atom::sprite::buffer_data g_sprite_buffer{};

static surge::vector<glm::mat4> g_sprite_models{};
static surge::vector<GLuint64> g_sprite_texture_handles{};

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
  if (!image_shader) {
    return static_cast<int>(surge::error::static_image_shader_creation);
  }
  g_image_shader = *image_shader;

  const auto sprite_shader{
      surge::renderer::create_shader_program("shaders/sprite.vert", "shaders/sprite.frag")};
  if (!sprite_shader) {
    return static_cast<int>(sprite_shader.error());
  }
  g_sprite_shader = *sprite_shader;
  g_sprite_buffer = surge::atom::sprite::create_buffers();
  g_sprite_models.reserve(8);
  g_sprite_texture_handles.reserve(8);

  auto img{*surge::files::load_image("resources/main_menu/background.png")};
  const auto img_handle{surge::atom::sprite::create_texture(img)};
  if (!img_handle) {
    surge::files::free_image(img);
    return static_cast<int>(img_handle.error());
  }
  g_sprite_texture_handles.push_back(*img_handle);
  surge::files::free_image(img);

  g_sprite_models.push_back(glm::scale(
      glm::translate(glm::mat4{1.0f}, glm::vec3{100.0, 100.0, 1.0}), glm::vec3{608.0, 174.0, 1.0}));

  surge::atom::sprite::make_resident(g_sprite_texture_handles);

  const auto text_shader{
      surge::renderer::create_shader_program("shaders/text.vert", "shaders/text.frag")};
  if (!text_shader) {
    return static_cast<int>(text_shader.error());
  }
  g_text_shader = *text_shader;

  // Load font cache
  surge::atom::text::font_name_vec_t fonts{"resources/fonts/ITC_Benguiat_Bold.ttf"};
  const auto text_buffer{surge::atom::text::create(fonts)};
  if (!text_buffer) {
    return static_cast<int>(text_buffer.error());
  }
  g_text_buffer = *text_buffer;

  const auto text_charmap{surge::atom::text::create_charmap(g_text_buffer, 50)};
  if (!text_charmap) {
    return static_cast<int>(text_buffer.error());
  }
  g_text_charmap = *text_charmap;

  return DTU::state::main_menu::load(g_command_queue, ww, wh);
}

extern "C" SURGE_MODULE_EXPORT auto on_unload(GLFWwindow *window) noexcept -> int {
  const auto unbind_callback_stat{DTU::unbind_callbacks(window)};
  if (unbind_callback_stat != 0) {
    return unbind_callback_stat;
  }

  surge::renderer::cleanup_shader_program(g_nonuniform_tile_shader);
  surge::renderer::cleanup_shader_program(g_image_shader);

  surge::atom::sprite::make_non_resident(g_sprite_texture_handles);
  surge::atom::sprite::destroy_buffers(g_sprite_buffer);
  surge::renderer::cleanup_shader_program(g_sprite_shader);

  surge::atom::text::destroy_charmap(g_text_charmap);
  surge::atom::text::terminate(g_text_buffer);
  surge::renderer::cleanup_shader_program(g_text_shader);

  switch (g_current_state_id) {
  case DTU::state_id::main_menu:
    return DTU::state::main_menu::unload(g_command_queue);
  default:
    break;
  }
}

extern "C" SURGE_MODULE_EXPORT auto draw() noexcept -> int {
  surge::atom::sprite::draw(g_sprite_shader, g_sprite_buffer, g_projection, g_view, g_sprite_models,
                            g_sprite_texture_handles);

  /*switch (g_current_state_id) {
  case DTU::state_id::main_menu:
    return DTU::state::main_menu::draw(
        DTU::state::main_menu::shader_indices{g_nonuniform_tile_shader, g_image_shader,
                                              g_text_shader},
        g_text_buffer, g_text_charmap, g_projection, g_view);
  default:
    break;
  }*/
  return 0;
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