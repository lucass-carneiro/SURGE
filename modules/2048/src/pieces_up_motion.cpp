#include "2048.hpp"
#include "logging.hpp"
#include "pieces.hpp"

#include <cstdint>

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#endif

void mod_2048::pieces::compress_up() noexcept {

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::pieces::compress_up");
#endif

  log_debug("Compressing up");

  auto &target_slots{get_piece_target_slots()};

  for (slot_t i = 0; i < 4; i++) {
    const auto element{get_element(board_element_type::column, i)};

    switch (element.config) {

    case board_element_configuration::OXOO:
      target_slots[element.data[1]] = i;
      should_add_new_piece(true);
      break;

    case board_element_configuration::OOXO:
      target_slots[element.data[2]] = i;
      should_add_new_piece(true);
      break;

    case board_element_configuration::OOOX:
      target_slots[element.data[3]] = i;
      should_add_new_piece(true);
      break;

    case board_element_configuration::XOXO:
      target_slots[element.data[2]] = i + 4;
      should_add_new_piece(true);
      break;

    case board_element_configuration::XOOX:
      target_slots[element.data[3]] = i + 4;
      should_add_new_piece(true);
      break;

    case board_element_configuration::OXXO:
      target_slots[element.data[1]] = i;
      target_slots[element.data[2]] = i + 4;
      should_add_new_piece(true);
      break;

    case board_element_configuration::OXOX:
      target_slots[element.data[1]] = i;
      target_slots[element.data[3]] = i + 4;
      should_add_new_piece(true);
      break;

    case board_element_configuration::OOXX:
      target_slots[element.data[2]] = i;
      target_slots[element.data[3]] = i + 4;
      should_add_new_piece(true);
      break;

    case board_element_configuration::XXOX:
      target_slots[element.data[3]] = i + 4 * 2;
      should_add_new_piece(true);
      break;

    case board_element_configuration::XOXX:
      target_slots[element.data[2]] = i + 4;
      target_slots[element.data[3]] = i + 4 * 2;
      should_add_new_piece(true);
      break;

    case board_element_configuration::OXXX:
      target_slots[element.data[1]] = i;
      target_slots[element.data[2]] = i + 4;
      target_slots[element.data[3]] = i + 4 * 2;
      should_add_new_piece(true);
      break;

    default:
      break;
    }
  }
}

void mod_2048::pieces::merge_up() noexcept {

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::pieces::merge_up");
#endif

  log_debug("Merging up");

  points_t round_points{0};

  auto &target_slots{get_piece_target_slots()};
  auto &exponents{get_piece_exponents()};
  auto &target_exponents{get_piece_target_exponents()};

  for (slot_t i = 0; i < 4; i++) {
    const auto element{get_element(board_element_type::column, i)};

    switch (element.config) {

    case board_element_configuration::XXOO:
      if (exponents[element.data[0]] == exponents[element.data[1]]) {
        target_slots[element.data[1]] = i;
        target_exponents[element.data[1]] += 1;
        round_points += 1 << target_exponents[element.data[1]];
        mark_stale(element.data[0]);
        should_add_new_piece(true);
      }
      break;

    case board_element_configuration::XXXO:
      if (exponents[element.data[0]] == exponents[element.data[1]]) {
        target_slots[element.data[1]] = i;
        target_slots[element.data[2]] = i + 4;
        target_exponents[element.data[1]] += 1;
        round_points += 1 << target_exponents[element.data[1]];
        mark_stale(element.data[0]);
        should_add_new_piece(true);
      } else if (exponents[element.data[1]] == exponents[element.data[2]]) {
        target_slots[element.data[2]] = i + 4;
        target_exponents[element.data[2]] += 1;
        round_points += 1 << target_exponents[element.data[2]];
        mark_stale(element.data[1]);
        should_add_new_piece(true);
      }
      break;

    case board_element_configuration::XXXX:
      if (exponents[element.data[0]] == exponents[element.data[1]]) {
        target_slots[element.data[1]] = i;
        target_slots[element.data[2]] = i + 4;
        target_slots[element.data[3]] = i + 4 * 2;
        target_exponents[element.data[1]] += 1;
        round_points += 1 << target_exponents[element.data[1]];
        mark_stale(element.data[0]);
        should_add_new_piece(true);
      } else if (exponents[element.data[1]] == exponents[element.data[2]]) {
        target_slots[element.data[2]] = i + 4;
        target_slots[element.data[3]] = i + 4 * 2;
        target_exponents[element.data[2]] += 1;
        round_points += 1 << target_exponents[element.data[2]];
        mark_stale(element.data[1]);
        should_add_new_piece(true);
      } else if (exponents[element.data[2]] == exponents[element.data[3]]) {
        target_slots[element.data[3]] = i + 4 * 2;
        target_exponents[element.data[3]] += 1;
        round_points += 1 << target_exponents[element.data[3]];
        mark_stale(element.data[2]);
        should_add_new_piece(true);
      }
      break;

    default:
      break;
    }
  }

  log_debug("Round points: %llu", round_points);
  add_game_score(round_points);
}