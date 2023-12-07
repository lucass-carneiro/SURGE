#include "logging.hpp"
#include "pieces.hpp"

void mod_2048::pieces::compress_right() noexcept {
  log_debug("Compressing right");

  auto &target_slots{get_piece_target_slots()};

  for (slot_t i = 0; i < 4; i++) {
    const auto element{get_element(board_element_type::row, i)};

    switch (element.config) {
    case board_element_configuration::XOOO:
      target_slots[element.data[0]] = 3 + i * 4;
      should_add_new_piece(true);
      break;

    case board_element_configuration::OXOO:
      target_slots[element.data[1]] = 3 + i * 4;
      should_add_new_piece(true);
      break;

    case board_element_configuration::OOXO:
      target_slots[element.data[2]] = 3 + i * 4;
      should_add_new_piece(true);
      break;

    case board_element_configuration::XXOO:
      target_slots[element.data[0]] = 2 + i * 4;
      target_slots[element.data[1]] = 3 + i * 4;
      should_add_new_piece(true);
      break;

    case board_element_configuration::XOXO:
      target_slots[element.data[0]] = 2 + i * 4;
      target_slots[element.data[2]] = 3 + i * 4;
      should_add_new_piece(true);
      break;

    case board_element_configuration::XOOX:
      target_slots[element.data[0]] = 2 + i * 4;
      should_add_new_piece(true);
      break;

    case board_element_configuration::OXXO:
      target_slots[element.data[1]] = 2;
      target_slots[element.data[2]] = 3;
      should_add_new_piece(true);
      break;

    case board_element_configuration::OXOX:
      target_slots[element.data[1]] = 2 + i * 4;
      should_add_new_piece(true);
      break;

    case board_element_configuration::XXXO:
      target_slots[element.data[0]] = 1 + i * 4;
      target_slots[element.data[1]] = 2 + i * 4;
      target_slots[element.data[2]] = 3 + i * 4;
      should_add_new_piece(true);
      break;

    case board_element_configuration::XXOX:
      target_slots[element.data[0]] = 1 + i * 4;
      target_slots[element.data[1]] = 2 + i * 4;
      should_add_new_piece(true);
      break;

    case board_element_configuration::XOXX:
      target_slots[element.data[0]] = 1 + i * 4;
      should_add_new_piece(true);
      break;

    default:
      break;
    }
  }
}

void mod_2048::pieces::merge_right() noexcept {
  log_debug("Merging right");

  auto &target_slots{get_piece_target_slots()};
  auto &exponents{get_piece_exponents()};
  auto &target_exponents{get_piece_target_exponents()};

  for (slot_t i = 0; i < 4; i++) {
    const auto element{get_element(board_element_type::row, i)};

    switch (element.config) {

    case board_element_configuration::OOXX:
      if (exponents[element.data[2]] == exponents[element.data[3]]) {
        target_slots[element.data[2]] = 3 + i * 4;
        target_exponents[element.data[2]] += 1;
        mark_stale(element.data[3]);
      }
      break;

    case board_element_configuration::OXXX:
      if (exponents[element.data[2]] == exponents[element.data[3]]) {
        target_slots[element.data[1]] = 2 + i * 4;
        target_slots[element.data[2]] = 3 + i * 4;
        target_exponents[element.data[2]] += 1;
        mark_stale(element.data[3]);
      } else if (exponents[element.data[1]] == exponents[element.data[2]]) {
        target_slots[element.data[1]] = 2 + i * 4;
        target_exponents[element.data[1]] += 1;
        mark_stale(element.data[2]);
      }
      break;

    case board_element_configuration::XXXX:
      if (exponents[element.data[2]] == exponents[element.data[3]]) {
        target_slots[element.data[0]] = 1 + i * 4;
        target_slots[element.data[1]] = 2 + i * 4;
        target_slots[element.data[2]] = 3 + i * 4;
        target_exponents[element.data[2]] += 1;
        mark_stale(element.data[3]);
      } else if (exponents[element.data[1]] == exponents[element.data[2]]) {
        target_slots[element.data[0]] = 1 + i * 4;
        target_slots[element.data[1]] = 2 + i * 4;
        target_exponents[element.data[1]] += 1;
        mark_stale(element.data[2]);
      } else if (exponents[element.data[0]] == exponents[element.data[1]]) {
        target_slots[element.data[0]] = 1 + i * 4;
        target_exponents[element.data[0]] += 1;
        mark_stale(element.data[1]);
      }
      break;

    default:
      break;
    }
  }
}