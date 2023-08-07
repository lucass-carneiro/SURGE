#include "default.hpp"
#include "logging.hpp"

// clang-format off
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
// clang-format on

surge::fonts::font_system_context globals::freetype_ctx{};
surge::fonts::charmap globals::char_map{};

extern "C" SURGE_MODULE_EXPORT auto on_load(GLFWwindow *window) noexcept -> bool {
  using namespace surge;
  using namespace globals;

  log_info("Loading default module");

  /*******************
   * Init font cache *
   *******************/
  log_info("Initializing font cache");
  const auto freetype_ctx_opt{fonts::init(window, "config.yaml")};
  if (!freetype_ctx_opt) {
    return false;
  }
  freetype_ctx = *freetype_ctx_opt;

  const auto char_map_opt{fonts::create_character_maps(freetype_ctx, 100)};
  if (!char_map_opt) {
    return false;
  }
  char_map = *char_map_opt;

  return true;
}

extern "C" SURGE_MODULE_EXPORT void on_unload() noexcept {
  using namespace surge;
  using namespace globals;

  log_info("Unloading default module");

  log_info("Terminating font cache");
  fonts::terminate(freetype_ctx);
}