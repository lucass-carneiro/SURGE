#include "2048.hpp"
#include "2048_board.hpp"
#include "2048_globals.hpp"
#include "2048_pieces.hpp"
#include "logging.hpp"
#include "renderer.hpp"
#include "window.hpp"

extern "C" SURGE_MODULE_EXPORT auto on_load(GLFWwindow *window) noexcept -> bool {
  using namespace surge;
  using namespace mod_2048;

  log_info("Loading 2048 data");

  log_info("Setting projection matrix");
  std::apply(globals::make_projection, window::get_dims(window));

  log_info("Loading board image");
  if (!board::make_img_ctx()) {
    return false;
  }

  log_info("Loading piece image");
  if (!pieces::make_img_ctx()) {
    return false;
  }

  return true;
}

extern "C" SURGE_MODULE_EXPORT void on_unload() noexcept { log_info("Unloading 2048 data"); }