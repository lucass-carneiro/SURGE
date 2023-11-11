#include "pieces.hpp"

#include "2048.hpp"
#include "logging.hpp"

#include <random>

void mod_2048::pieces::draw() noexcept {
  using namespace surge::atom;

  const auto &img_shader{get_img_shader()};
  const auto &pieces_buffer{get_pieces_buffer()};

  const auto &positions{get_piece_positions()};
  const auto &exponents{get_piece_exponents()};

  const auto &texture_origins{get_piece_texture_origins()};

  const auto &slot_size{get_slot_size()};

  const glm::vec3 scale{slot_size, slot_size, 1.0f};
  const glm::vec2 img_scale{slot_size, slot_size};

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
  for (const auto &i : collection) {
    if (i.second == value) {
      return true;
    }
  }
  return false;
}

auto mod_2048::pieces::create_random(exponent_t last_exponent) noexcept -> piece_id_t {
  const auto &slots{get_piece_slots()};

  if (slots.size() == 16) {
    log_warn("pieces::create_random failed because the board is full. Returning piece ID 16");
    return 16;
  }

  static std::mt19937 gen{std::random_device{}()};
  static std::uniform_int_distribution<id_t> exp_distrib(1, last_exponent);
  static std::uniform_int_distribution<id_t> pos_distrib(0, 15);

  const auto exp{exp_distrib(gen)};

  // Find free slot
  auto randomized_slot{pos_distrib(gen)};

  while (has_value(randomized_slot, slots)) {
    randomized_slot = pos_distrib(gen);
  }

  return create_piece(exp, randomized_slot);
}
