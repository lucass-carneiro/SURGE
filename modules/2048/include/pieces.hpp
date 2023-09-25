#ifndef SURGE_MODULE_2048_PIECES_HPP
#define SURGE_MODULE_2048_PIECES_HPP

#include "renderer.hpp"

#include <array>
#include <cstdint>
#include <ranges>

namespace mod_2048::pieces {

using std::array;
using u8 = std::uint8_t;

extern u8 num_pieces;

struct positions {
  array<float, 16> x{-1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,
                     -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f};
  array<float, 16> y{-1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,
                     -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f};
};

using exponents = array<u8, 16>;
using slots = array<u8, 16>;
using occupation = array<u8, 16>;

auto add(u8 slot, u8 exponent) noexcept -> bool;
auto add_random() noexcept -> bool;

void compress_right() noexcept;

void draw() noexcept;
void update(double dt) noexcept;

auto make_img_ctx() noexcept -> bool;
auto get_img_ctx() noexcept -> const surge::renderer::image::context &;

auto get_positions() noexcept -> const positions &;
auto get_exponents() noexcept -> const exponents &;
auto get_slots() noexcept -> const slots &;
auto get_occupation() noexcept -> const occupation &;

} // namespace mod_2048::pieces

#endif // SURGE_MODULE_2048_PIECES_HPP