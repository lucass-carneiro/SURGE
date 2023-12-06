#ifndef SURGE_MODULE_2048_PIECES
#define SURGE_MODULE_2048_PIECES

#include "allocators.hpp"

#include <EASTL/deque.h>
#include <EASTL/fixed_hash_map.h>
#include <glm/matrix.hpp>

namespace mod_2048::pieces {

using exponent_t = unsigned short;
using slot_t = unsigned short;

using piece_id_t = unsigned short;
using piece_id_queue_t = eastl::deque<piece_id_t, surge::allocators::eastl::gp_allocator>;

using piece_pos_t = glm::vec3;
using piece_positions_t = eastl::fixed_hash_map<piece_id_t, piece_pos_t, 16, 17, false>;

using piece_exponents_t = eastl::fixed_hash_map<piece_id_t, exponent_t, 16, 17, false>;

using piece_slots_t = eastl::fixed_hash_map<piece_id_t, slot_t, 16, 17, false>;

enum class board_element_type : unsigned short { row, column };

enum board_element_configuration : unsigned short {
  // 0 pieces
  OOOO,

  // 1 Piece combos
  XOOO,
  OXOO,
  OOXO,
  OOOX,

  // 2 Piece combos
  XXOO,
  XOXO,
  XOOX,
  OXXO,
  OXOX,
  OOXX,

  // 3 Piece combos
  XXXO,
  XXOX,
  XOXX,
  OXXX,

  // 4 Piece combos
  XXXX
};

struct board_element {
  std::array<piece_id_t, 4> data;
  unsigned short size;
  board_element_configuration config;
};

struct board_address {
  slot_t row;
  slot_t col;
};

auto get_piece_positions() noexcept -> piece_positions_t &;

auto get_piece_exponents() noexcept -> piece_exponents_t &;
auto get_piece_target_exponents() noexcept -> piece_exponents_t &;

auto get_piece_slots() noexcept -> piece_slots_t &;
auto get_piece_target_slots() noexcept -> piece_slots_t &;

auto create_piece(exponent_t exponent, slot_t slot) noexcept -> piece_id_t;
void delete_piece(piece_id_t piece_id) noexcept;

auto create_random(exponent_t last_exponent = 2) noexcept -> piece_id_t;

auto idle() noexcept -> bool;

auto deflatten_slot(slot_t slot) noexcept -> board_address;
auto get_element(board_element_type type, slot_t value) noexcept -> board_element;

void compress_right() noexcept;
void merge_right() noexcept;

void mark_stale(piece_id_t piece) noexcept;
void remove_stale() noexcept;

void draw() noexcept;

void update_positions(double dt) noexcept;
void update_exponents() noexcept;

} // namespace mod_2048::pieces

#endif // SURGE_MODULE_2048_PIECES