#include "2048.hpp"
#include "pieces.hpp"
#include "static_image.hpp"

auto draw() noexcept -> std::uint32_t {
  using namespace mod_2048;
  using namespace surge::atom;

  // Draw board
  static_image::draw(get_img_shader(), get_board_buffer(), get_board_draw_data());

  pieces::draw();

  return 0;
}