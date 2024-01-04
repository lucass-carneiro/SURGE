#include "2048.hpp"

#ifdef SURGE_BUILD_TYPE_Debug
#  include "debug_window.hpp"
#endif

#include "pieces.hpp"
#include "score.hpp"
#include "static_image.hpp"
#include "text.hpp"

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#endif

auto draw() noexcept -> std::uint32_t {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::draw()");
#endif

  using namespace mod_2048;
  using namespace surge::atom;

  // Draw board
  static_image::draw(get_img_shader(), get_board_buffer(), get_board_draw_data());

  pieces::draw();

#ifdef SURGE_BUILD_TYPE_Debug
  debug_window::draw();
#endif

  score::draw();

  return 0;
}