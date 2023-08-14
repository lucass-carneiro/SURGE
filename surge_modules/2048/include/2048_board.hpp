#ifndef SURGE_2048_BOARD_HPP
#define SURGE_2048_BOARD_HPP

#include "renderer.hpp"

#include <cstdint>

namespace mod_2048::board {

auto make_img_ctx() noexcept -> bool;
auto get_img_ctx() noexcept -> const surge::renderer::image::context &;

auto get_slot_x(std::uint8_t slot_idx) noexcept -> float;
auto get_slot_y(std::uint8_t slot_idx) noexcept -> float;

void draw() noexcept;

} // namespace mod_2048::board

#endif // SURGE_2048_BOARD_HPP