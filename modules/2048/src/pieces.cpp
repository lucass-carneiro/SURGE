#include "pieces.hpp"

#include "board.hpp"
#include "globals.hpp"
#include "logging.hpp"
#include "options.hpp"

#include <random>

namespace mod_2048::pieces {

static surge::renderer::image::context img_ctx{};

static constexpr const std::array<float, 11> texture_origin_x
    = {1.0, 104.0, 207.0, 310.0, 1.0, 104.0, 207.0, 310.0, 1.0, 104.0, 207.0};

static constexpr const std::array<float, 11> texture_origin_y
    = {1.0, 1.0, 1.0, 1.0, 104.0, 104.0, 104.0, 104.0, 207.0, 207.0, 207.0};

static const glm::vec2 slot_size{102.0f, 102.0f};

} // namespace mod_2048::pieces

mod_2048::pieces::u8 mod_2048::pieces::num_pieces{0};

static mod_2048::pieces::positions poss{};
static mod_2048::pieces::exponents exps{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static mod_2048::pieces::slots slts{16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16};
static mod_2048::pieces::occupation ocp{16, 16, 16, 16, 16, 16, 16, 16,
                                        16, 16, 16, 16, 16, 16, 16, 16};

auto mod_2048::pieces::add(u8 slot, u8 exponent) noexcept -> bool {
#ifdef SURGE_DEBUG_MEMORY
  if (num_pieces < 16 && slot < 16) {
    poss.x[num_pieces] = board::get_slot_x(slot);
    poss.y[num_pieces] = board::get_slot_y(slot);
    exps[num_pieces] = exponent;
    slts[num_pieces] = slot;
    ocp[slot] = num_pieces;
    num_pieces++;
    return true;
  } else {
    return false;
  }
#else
  pos.x[num_pieces] = board::get_slot_x(slot);
  pos.y[num_pieces] = board::get_slot_y(slot);
  exps[num_pieces] = exponent;
  slts[num_pieces] = slot;
  num_pieces++;
  return true;
#endif
}

auto mod_2048::pieces::add_random() noexcept -> bool {
  static std::mt19937 gen{std::random_device{}()};
  static std::uniform_int_distribution<id_t> exp_distrib(1, 2);
  static std::uniform_int_distribution<id_t> pos_distrib(0, 15);

  const auto exp{exp_distrib(gen)};
  auto slot{pos_distrib(gen)};

  while (ocp[slot] != 16) {
    slot = pos_distrib(gen);
  }

  log_info("Adding piece to slot %i exponent %i", slot, exp);
  return add(slot, exp);
}

void mod_2048::pieces::compress_right() noexcept {
  // do nothing
}

void mod_2048::pieces::draw() noexcept {
  using mod_2048::globals::get_projection;
  using mod_2048::globals::get_view;
  using surge::renderer::image::draw;
  using surge::renderer::image::draw_context;

  for (u8 id = 0; id < num_pieces; id++) {
    const auto exponent{exps[id]};

    const glm::vec2 text_org{texture_origin_x.at(exponent - 1), texture_origin_y.at(exponent - 1)};

    const glm::vec3 p{poss.x[id], poss.y[id], 0.2f};
    const glm::vec3 s{slot_size, 1.0f};

    draw(img_ctx, draw_context{get_projection(), get_view(), p, s, text_org, slot_size});
  }
}

void mod_2048::pieces::update(double) noexcept {
  // do nothing
}

auto mod_2048::pieces::make_img_ctx() noexcept -> bool {
  using surge::renderer::image::create;

  const auto piece_img_ctx_opt{create("resources/pieces.png")};
  if (!piece_img_ctx_opt) {
    return false;
  } else {
    img_ctx = *piece_img_ctx_opt;
    return true;
  }
}

auto mod_2048::pieces::get_img_ctx() noexcept -> const surge::renderer::image::context & {
  return img_ctx;
}

auto mod_2048::pieces::get_positions() noexcept -> const positions & { return poss; }
auto mod_2048::pieces::get_exponents() noexcept -> const exponents & { return exps; }
auto mod_2048::pieces::get_slots() noexcept -> const slots & { return slts; }
auto mod_2048::pieces::get_occupation() noexcept -> const occupation & { return ocp; }