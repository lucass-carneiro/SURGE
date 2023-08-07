#include "default.hpp"
#include "logging.hpp"

// clang-format off
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
// clang-format on

std::optional<surge::fonts::font_system_context> globals::freetype_ctx{};
std::optional<surge::fonts::charmap> globals::char_map{};

std::optional<surge::renderer::image::context> globals::sheep_img{};
surge::renderer::image::draw_context globals::sheep_dc{};

extern "C" {

SURGE_MODULE_EXPORT auto on_load(GLFWwindow *window) noexcept -> bool {
  using namespace surge;
  using namespace globals;

  log_info("Loading default module");

  /*******************
   * Init font cache *
   *******************/
  log_info("Initializing font cache");
  freetype_ctx = fonts::init(window, "config.yaml");
  if (!freetype_ctx) {
    return false;
  }

  const auto [ww, wh] = window::get_dims(window);
  sheep_dc.projection = glm::ortho(0.0f, ww, wh, 0.0f);
  sheep_dc.view = glm::lookAt(glm::vec3{0.0f, 0.0f, 1.0f}, glm::vec3{0.0f, 0.0f, 0.0f},
                              glm::vec3{0.0f, 1.0f, 0.0f});

  char_map = fonts::create_character_maps(*freetype_ctx, 100);
  if (!char_map) {
    return false;
  }

  /*******************
   * Load sheep image *
   *******************/
  log_info("Loding sheep image");
  sheep_img = renderer::image::create("sheep.png");
  if (!sheep_img) {
    return false;
  }

  return true;
}

SURGE_MODULE_EXPORT void on_unload() noexcept {
  using namespace surge;
  using namespace globals;

  log_info("Unloading default module");

  log_info("Terminating font cache");
  fonts::terminate(*freetype_ctx);
}
}