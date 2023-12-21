#include "2048.hpp"
#include "logging.hpp"
#include "pieces.hpp"

#include <cstdint>

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#endif

void mod_2048::pieces::compress_down() noexcept {

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::pieces::compress_down");
#endif

  log_debug("Compressing down");

  auto &target_slots{get_piece_target_slots()};

  for (slot_t i = 0; i < 4; i++) {
    const auto element{get_element(board_element_type::column, i)};

    switch (element.config) {

    case board_element_configuration::XOOO:
      target_slots[element.data[0]] = i + 4 * 3;
      should_add_new_piece(true);
      break;

    case board_element_configuration::OXOO:
      target_slots[element.data[1]] = i + 4 * 3;
      should_add_new_piece(true);
      break;

    case board_element_configuration::OOXO:
      target_slots[element.data[2]] = i + 4 * 3;
      should_add_new_piece(true);
      break;

    case board_element_configuration::OXOX:
      target_slots[element.data[1]] = i + 4 * 2;
      should_add_new_piece(true);
      break;

    case board_element_configuration::XOOX:
      target_slots[element.data[0]] = i + 4 * 2;
      should_add_new_piece(true);
      break;

    case board_element_configuration::OXXO:
      target_slots[element.data[1]] = i + 4 * 2;
      target_slots[element.data[2]] = i + 4 * 3;
      should_add_new_piece(true);
      break;

    case board_element_configuration::XXOO:
      target_slots[element.data[0]] = i + 4 * 2;
      target_slots[element.data[1]] = i + 4 * 3;
      should_add_new_piece(true);
      break;

    case board_element_configuration::XOXO:
      target_slots[element.data[0]] = i + 4 * 2;
      target_slots[element.data[2]] = i + 4 * 3;
      should_add_new_piece(true);
      break;

    case board_element_configuration::XOXX:
      target_slots[element.data[0]] = i + 4;
      should_add_new_piece(true);
      break;

    case board_element_configuration::XXOX:
      target_slots[element.data[0]] = i + 4;
      target_slots[element.data[1]] = i + 4 * 2;
      should_add_new_piece(true);
      break;

    case board_element_configuration::XXXO:
      target_slots[element.data[0]] = i + 4;
      target_slots[element.data[1]] = i + 4 * 2;
      target_slots[element.data[2]] = i + 4 * 3;
      should_add_new_piece(true);
      break;

    default:
      break;
    }
  }
}

void mod_2048::pieces::merge_down() noexcept {

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::pieces::merge_down");
#endif

  log_debug("Merging down");

  points_t round_points{0};

  auto &target_slots{get_piece_target_slots()};
  auto &exponents{get_piece_exponents()};
  auto &target_exponents{get_piece_target_exponents()};

  for (slot_t i = 0; i < 4; i++) {
    const auto element{get_element(board_element_type::column, i)};

    switch (element.config) {

    case board_element_configuration::OOXX:
      if (exponents[element.data[2]] == exponents[element.data[3]]) {
        target_slots[element.data[2]] = i + 4 * 3;
        target_exponents[element.data[2]] += 1;
        round_points += 1 << target_exponents[element.data[2]];
        mark_stale(element.data[3]);
        should_add_new_piece(true);
      }
      break;

    case board_element_configuration::OXXX:
      if (exponents[element.data[2]] == exponents[element.data[3]]) {
        target_slots[element.data[1]] = i + 4 * 2;
        target_slots[element.data[2]] = i + 4 * 3;
        target_exponents[element.data[2]] += 1;
        round_points += 1 << target_exponents[element.data[2]];
        mark_stale(element.data[3]);
        should_add_new_piece(true);
      } else if (exponents[element.data[1]] == exponents[element.data[2]]) {
        target_slots[element.data[1]] = i + 4 * 2;
        target_exponents[element.data[1]] += 1;
        round_points += 1 << target_exponents[element.data[1]];
        mark_stale(element.data[2]);
        should_add_new_piece(true);
      }
      break;

    case board_element_configuration::XXXX:
      if (exponents[element.data[2]] == exponents[element.data[3]]) {
        target_slots[element.data[0]] = i + 4;
        target_slots[element.data[1]] = i + 4 * 2;
        target_slots[element.data[2]] = i + 4 * 3;
        target_exponents[element.data[2]] += 1;
        round_points += 1 << target_exponents[element.data[2]];
        mark_stale(element.data[3]);
        should_add_new_piece(true);
      } else if (exponents[element.data[1]] == exponents[element.data[2]]) {
        target_slots[element.data[0]] = i + 4;
        target_slots[element.data[1]] = i + 4 * 2;
        target_exponents[element.data[1]] += 1;
        round_points += 1 << target_exponents[element.data[1]];
        mark_stale(element.data[2]);
        should_add_new_piece(true);
      } else if (exponents[element.data[0]] == exponents[element.data[1]]) {
        target_slots[element.data[0]] = i + 4;
        target_exponents[element.data[0]] += 1;
        round_points += 1 << target_exponents[element.data[0]];
        mark_stale(element.data[1]);
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