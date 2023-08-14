#include "2048_piece.hpp"

#include "2048_board.hpp"
#include "2048_globals.hpp"

// clang-format off
#include "logging.hpp"
#include "renderer.hpp"
// clang-format on

// clang-format off
#include <EASTL/fixed_hash_map.h>
#include <EASTL/fixed_list.h>
// clang-format on

#include <EASTL/sort.h>
#include <array>
#include <random>

namespace mod_2048::piece {

static surge::renderer::image::context img_ctx{};

static constexpr const std::array<float, 11> texture_origin_x
    = {1.0, 104.0, 207.0, 310.0, 1.0, 104.0, 207.0, 310.0, 1.0, 104.0, 207.0};

static constexpr const std::array<float, 11> texture_origin_y
    = {1.0, 1.0, 1.0, 1.0, 104.0, 104.0, 104.0, 104.0, 207.0, 207.0, 207.0};

static const glm::vec2 slot_size{102.0f, 102.0f};

// The number of buckets is the closest prime to 16
template <typename K, typename V> using map_t = eastl::fixed_hash_map<K, V, 16, 17, false>;

template <typename T> using slist_t = eastl::fixed_list<T, 16, false>;

static slist_t<id_t> available_IDs{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
static map_t<id_t, id_t> slots{};
static map_t<id_t, id_t> exponents{};

} // namespace mod_2048::piece

void mod_2048::piece::add(std::uint8_t slot, std::uint8_t exponent) noexcept {
  if (!available_IDs.empty()) {
    const auto id{available_IDs.front()};
    slots[id] = slot;
    exponents[id] = exponent;
    available_IDs.pop_front();
  }
}

void mod_2048::piece::remove(id_t id) noexcept {
  slots.erase(id);
  exponents.erase(id);
  available_IDs.push_back(id);
}

auto mod_2048::piece::is_occupied(id_t slot) noexcept -> bool {
  for (const auto &p : slots) {
    if (p.second == slot) {
      return true;
    }
  }
  return false;
}

void mod_2048::piece::add_random() noexcept {
  static std::mt19937 gen{std::random_device{}()};
  static std::uniform_int_distribution<id_t> exp_distrib(1, 2);
  static std::uniform_int_distribution<id_t> pos_distrib(0, 15);

  const auto exp{exp_distrib(gen)};
  auto slot{pos_distrib(gen)};

  while (is_occupied(slot)) {
    slot = pos_distrib(gen);
  }

  log_info("Adding piece to slot %i exponent %i", slot, exp);
  add(slot, exp);
}

void mod_2048::piece::draw() noexcept {
  using mod_2048::globals::get_projection;
  using mod_2048::globals::get_view;
  using surge::renderer::image::draw;
  using surge::renderer::image::draw_context;

  for (const auto &slot : slots) {
    const auto piece_id{slot.first};
    const auto slot_idx{slot.second};
    const auto exponent{exponents[piece_id]};

    const glm::vec2 text_org{texture_origin_x.at(exponent - 1), texture_origin_y.at(exponent - 1)};

    const glm::vec3 pos{board::get_slot_x(slot_idx), board::get_slot_y(slot_idx), 0.2f};
    const glm::vec3 scale{slot_size, 1.0f};

    draw(img_ctx, draw_context{get_projection(), get_view(), pos, scale, text_org, slot_size});
  }
}

auto mod_2048::piece::make_img_ctx() noexcept -> bool {
  using surge::renderer::image::create;

  const auto piece_img_ctx_opt{create("resources/pieces.png")};
  if (!piece_img_ctx_opt) {
    return false;
  } else {
    img_ctx = *piece_img_ctx_opt;
    return true;
  }
}

auto mod_2048::piece::get_img_ctx() noexcept -> const surge::renderer::image::context & {
  return img_ctx;
}
