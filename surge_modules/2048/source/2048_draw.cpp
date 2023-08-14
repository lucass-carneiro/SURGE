#include "2048.hpp"
#include "2048_board.hpp"
#include "2048_piece.hpp"

extern "C" SURGE_MODULE_EXPORT void draw(GLFWwindow *) noexcept {
  using namespace mod_2048;

  board::draw();
  piece::draw();
}