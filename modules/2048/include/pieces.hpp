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

using piece_command_code_t = std::uint8_t;

enum class commands : piece_command_code_t {
  change_exp_to_1 = 1,
  change_exp_to_2 = 2,
  change_exp_to_3 = 3,
  change_exp_to_4 = 4,
  change_exp_to_5 = 5,
  change_exp_to_6 = 6,
  change_exp_to_7 = 7,
  change_exp_to_8 = 8,
  change_exp_to_9 = 9,
  change_exp_to_10 = 10,
  change_exp_to_11 = 11,
  move_up = 12,
  move_down = 13,
  move_left = 14,
  move_right = 15
};

// Packed into a 16 bit interger as ID | command
using command_t = std::uint16_t;
using piece_command_queue_t = eastl::deque<command_t, surge::allocators::eastl::gp_allocator>;

auto get_piece_positions() noexcept -> piece_positions_t &;
auto get_piece_exponents() noexcept -> piece_exponents_t &;
auto get_piece_slots() noexcept -> piece_slots_t &;
auto get_piece_target_slots() noexcept -> piece_slots_t &;
auto get_piece_command_queue() noexcept -> piece_command_queue_t &;

auto get_command_desc(const commands &cmd) noexcept -> const char *;

auto create_piece(exponent_t exponent, slot_t slot) noexcept -> piece_id_t;
void delete_piece(piece_id_t piece_id) noexcept;

auto create_random(exponent_t last_exponent = 2) noexcept -> piece_id_t;

enum class move_direction { up, down, left, right };

void draw() noexcept;

void update(double dt) noexcept;

} // namespace mod_2048::pieces

#endif // SURGE_MODULE_2048_PIECES