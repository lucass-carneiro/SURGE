#ifndef SURGE_MODULE_2048_PIECES
#define SURGE_MODULE_2048_PIECES

#include "allocators.hpp"

#include <EASTL/deque.h>
#include <EASTL/fixed_hash_map.h>
#include <cstdint>
#include <glm/matrix.hpp>

namespace mod_2048::pieces {

using exponent_t = std::uint8_t;
using slot_t = std::uint8_t;

using piece_id_t = std::uint8_t;
using piece_id_queue_t = eastl::deque<piece_id_t, surge::allocators::eastl::gp_allocator>;

using piece_pos_t = glm::vec3;
using piece_positions_t = eastl::fixed_hash_map<piece_id_t, piece_pos_t, 16, 17, false>;

using piece_exponents_t = eastl::fixed_hash_map<piece_id_t, exponent_t, 16, 17, false>;

using piece_slots_t = eastl::fixed_hash_map<piece_id_t, slot_t, 16, 17, false>;

auto get_piece_positions() noexcept -> const piece_positions_t &;
auto get_piece_exponents() noexcept -> const piece_exponents_t &;
auto get_piece_slots() noexcept -> const piece_slots_t &;

auto create_piece(exponent_t exponent, slot_t slot) noexcept -> piece_id_t;
void delete_piece(piece_id_t piece_id) noexcept;

auto create_random(exponent_t last_exponent = 2) noexcept -> piece_id_t;

void draw() noexcept;

} // namespace mod_2048::pieces

#endif // SURGE_MODULE_2048_PIECES