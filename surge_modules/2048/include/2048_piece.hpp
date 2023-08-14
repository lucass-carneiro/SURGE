#ifndef SURGE_2048_PIECE_HPP
#define SURGE_2048_PIECE_HPP

#include "renderer.hpp"

#include <cstdint>

namespace mod_2048::piece {

using id_t = std::uint8_t;

auto make_img_ctx() noexcept -> bool;
auto get_img_ctx() noexcept -> const surge::renderer::image::context &;

void add(id_t slot, id_t exponent) noexcept;
void remove(id_t id) noexcept;

void add_random() noexcept;

void draw() noexcept;

} // namespace mod_2048::piece

#endif // SURGE_2048_PIECE_HPP