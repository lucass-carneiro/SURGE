#include "2048.hpp"
#include "2048_board.hpp"
#include "2048_debug_window.hpp"
#include "2048_pieces.hpp"

extern "C" SURGE_MODULE_EXPORT void draw(GLFWwindow *) noexcept {
  using namespace mod_2048;

  board::draw();
  pieces::draw();
  debug_window::draw();
}