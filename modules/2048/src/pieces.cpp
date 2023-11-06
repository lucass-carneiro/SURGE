#include "pieces.hpp"

#include "2048.hpp"

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
    piece.region_origin = texture_origins[exponent - 1];

    static_image::draw(img_shader, pieces_buffer, piece);
  }
}