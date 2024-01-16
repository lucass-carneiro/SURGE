#include "pieces.hpp"

#include "2048.hpp"
#include "player/logging.hpp"

#include <random>

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#endif

void mod_2048::pieces::draw() noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::pieces::draw()");
#endif

  using namespace surge::atom;

  const auto &img_shader{get_img_shader()};
  const auto &pieces_buffer{get_pieces_buffer()};

  const auto &positions{get_piece_positions()};
  const auto &exponents{get_piece_exponents()};

  const auto &texture_origins{get_piece_texture_origins()};

  const auto &slot_size{get_slot_size()};

  draw_t piece;
  piece.projection = get_projection();
  piece.view = get_view();
  piece.scale = glm::vec3{slot_size, slot_size, 1.0f};
  piece.region_dims = glm::vec2{slot_size, slot_size};
  piece.h_flip = false;
  piece.v_flip = false;

  for (const auto &elm : positions) {
    const auto id{elm.first};
    const auto exponent{exponents.at(id)};

    piece.pos = elm.second;
    piece.region_origin = texture_origins.at(exponent - 1);

    static_image::draw(img_shader, pieces_buffer, piece);
  }
}

template <typename T, typename U> static auto has_value(const T value, const U &collection)
    -> bool {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::pieces::has_value");
#endif

  for (const auto &i : collection) {
    if (i.second == value) {
      return true;
    }
  }
  return false;
}

auto mod_2048::pieces::create_random(exponent_t last_exponent) noexcept -> piece_id_t {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::pieces::create_random");
#endif

  log_debug("Adding random piece");

  const auto &slots{get_piece_slots()};

  if (slots.size() == 16) {
    log_debug("pieces::create_random failed because the board is full. Returning piece ID 16");
    return 16;
  }

  static std::mt19937 gen{std::random_device{}()};
  static std::uniform_int_distribution<unsigned short> exp_distrib(1, last_exponent);
  static std::uniform_int_distribution<unsigned short> pos_distrib(0, 15);

  const auto exp{exp_distrib(gen)};

  // Find free slot
  auto randomized_slot{pos_distrib(gen)};

  while (has_value(randomized_slot, slots)) {
    randomized_slot = pos_distrib(gen);
  }

  return create_piece(exp, randomized_slot);
}

auto mod_2048::pieces::idle() noexcept -> bool {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::pieces::idle");
#endif

  auto &slots{get_piece_slots()};
  auto &target_slots{get_piece_target_slots()};

  for (const auto &s : slots) {
    if (s.second != target_slots.at(s.first)) {
      return false;
    }
  }

  return true;
}

auto mod_2048::pieces::deflatten_slot(slot_t slot) noexcept -> board_address {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::pieces::deflatten_slot");
#endif

  switch (slot) {
  case 0:
    return board_address{0, 0};
  case 1:
    return board_address{0, 1};
  case 2:
    return board_address{0, 2};
  case 3:
    return board_address{0, 3};
  case 4:
    return board_address{1, 0};
  case 5:
    return board_address{1, 1};
  case 6:
    return board_address{1, 2};
  case 7:
    return board_address{1, 3};
  case 8:
    return board_address{2, 0};
  case 9:
    return board_address{2, 1};
  case 10:
    return board_address{2, 2};
  case 11:
    return board_address{2, 3};
  case 12:
    return board_address{3, 0};
  case 13:
    return board_address{3, 1};
  case 14:
    return board_address{3, 2};
  case 15:
    return board_address{3, 3};
  default:
    return board_address{16, 16};
  }
}

auto mod_2048::pieces::get_element(board_element_type type, slot_t value) noexcept
    -> board_element {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::pieces::get_element");
#endif

  board_element element{{16, 16, 16, 16}, 0, board_element_configuration::OOOO};

  // Get data and size
  for (const auto &slot : get_piece_slots()) {
    const auto slot_coords{deflatten_slot(slot.second)};

    if (type == board_element_type::row && slot_coords.row == value) {
      element.data.at(slot_coords.col) = slot.first;
      element.size += 1;

    } else if (type == board_element_type::column && slot_coords.col == value) {
      element.data.at(slot_coords.row) = slot.first;
      element.size += 1;
    }
  }

  // Get get configuration
  if (element.size == 0) {
    return element;
  } else if (element.size == 1) {
    if (element.data[0] != 16) {
      element.config = board_element_configuration::XOOO;
    } else if (element.data[1] != 16) {
      element.config = board_element_configuration::OXOO;
    } else if (element.data[2] != 16) {
      element.config = board_element_configuration::OOXO;
    } else {
      element.config = board_element_configuration::OOOX;
    }
  } else if (element.size == 4) {
    element.config = board_element_configuration::XXXX;
  } else if (element.size == 3) {
    if (element.data[0] != 16 && element.data[1] != 16 && element.data[2] != 16) {
      element.config = board_element_configuration::XXXO;
    } else if (element.data[0] != 16 && element.data[1] != 16 && element.data[3] != 16) {
      element.config = board_element_configuration::XXOX;
    } else if (element.data[0] != 16 && element.data[2] != 16 && element.data[3] != 16) {
      element.config = board_element_configuration::XOXX;
    } else {
      element.config = board_element_configuration::OXXX;
    }
  } else if (element.size == 2) {
    if (element.data[0] != 16 && element.data[1] != 16) {
      element.config = board_element_configuration::XXOO;
    } else if (element.data[0] != 16 && element.data[2] != 16) {
      element.config = board_element_configuration::XOXO;
    } else if (element.data[0] != 16 && element.data[3] != 16) {
      element.config = board_element_configuration::XOOX;
    } else if (element.data[1] != 16 && element.data[2] != 16) {
      element.config = board_element_configuration::OXXO;
    } else if (element.data[1] != 16 && element.data[3] != 16) {
      element.config = board_element_configuration::OXOX;
    } else {
      element.config = board_element_configuration::OOXX;
    }
  }

  return element;
}

void mod_2048::pieces::update_positions(double) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::pieces::update_positions");
#endif

  using std::abs, std::sqrt;

  // These values must be fine tuned together
  const float v{(get_slot_delta() * 0.125f)}; // Divide by dt
  constexpr const float threshold{2.5f};

  auto &positions{get_piece_positions()};

  auto &slots{get_piece_slots()};
  auto &target_slots{get_piece_target_slots()};

  for (const auto &s : slots) {
    auto piece_id{s.first};

    auto src_slot{s.second};
    auto tgt_slot{target_slots.at(piece_id)};

    // Move pieces
    if (tgt_slot != src_slot) {
      const auto curr_pos{positions.at(piece_id)};
      const auto tgt_slot_pos{get_slot_coords().at(tgt_slot)};

      const auto delta_r{tgt_slot_pos - curr_pos};
      const auto delta_r_length{sqrt(glm::dot(delta_r, delta_r))};

      // Stopping condition
      if (abs(delta_r_length) < threshold) {
        slots.at(piece_id) = tgt_slot;
        positions.at(piece_id) = tgt_slot_pos;
      } else {
        const auto n_r{delta_r / delta_r_length};
        const auto r_next{curr_pos + v * n_r}; // Multiply by dt. dt cancels
        positions.at(piece_id) = r_next;
      }
    }
  }
}

void mod_2048::pieces::update_exponents() noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("mod_2048::pieces::update_exponents");
#endif

  auto &exponents{get_piece_exponents()};
  auto &target_exponents{get_piece_target_exponents()};

  for (const auto &e : exponents) {
    auto piece_id{e.first};

    auto src_exp{e.second};
    auto tgt_exp{target_exponents.at(piece_id)};

    if (src_exp != tgt_exp) {
      exponents.at(piece_id) = tgt_exp;
    }
  }
}