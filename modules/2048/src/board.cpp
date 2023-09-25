#include "board.hpp"

#include "globals.hpp"

#include <array>
#include <glm/glm.hpp>

namespace mod_2048::board {

static surge::renderer::image::context img_ctx{};

static constexpr const std::array<float, 16> slots_x
    = {9.0, 120.0, 231.0, 342.0, 9.0, 120.0, 231.0, 342.0,
       9.0, 120.0, 231.0, 342.0, 9.0, 120.0, 231.0, 342.0};

static constexpr const std::array<float, 16> slots_y
    = {9.0,   9.0,   9.0,   9.0,   120.0, 120.0, 120.0, 120.0,
       231.0, 231.0, 231.0, 231.0, 342.0, 342.0, 342.0, 342.0};

} // namespace mod_2048::board

auto mod_2048::board::make_img_ctx() noexcept -> bool {
  using surge::renderer::image::create;

  const auto board_img_ctx_opt{create("resources/board_debug.png")};
  if (!board_img_ctx_opt) {
    return false;
  } else {
    img_ctx = *board_img_ctx_opt;
    return true;
  }
}

auto mod_2048::board::get_img_ctx() noexcept -> const surge::renderer::image::context & {
  return img_ctx;
}

void mod_2048::board::draw() noexcept {
  using mod_2048::globals::get_projection;
  using mod_2048::globals::get_view;
  using surge::renderer::image::draw;
  using surge::renderer::image::draw_context;

  draw(img_ctx,
       draw_context{get_projection(), get_view(), glm::vec3{0.0f, 0.0, 0.1f},
                    glm::vec3{img_ctx.dimentions, 1.0f}, glm::vec2{0.0f}, img_ctx.dimentions});
}

auto mod_2048::board::get_slot_x(std::uint8_t slot_idx) noexcept -> float {
  return slots_x.at(slot_idx);
}

auto mod_2048::board::get_slot_y(std::uint8_t slot_idx) noexcept -> float {
  return slots_y.at(slot_idx);
}