#include "score.hpp"

#include "2048.hpp"

#include <algorithm>
#include <array>
#include <limits>

// clang-format off
static const std::array<glm::vec2, 10> g_number_texture_origins {
  glm::vec2{1.0f , 1.0f},
  glm::vec2{18.0f, 1.0f},
  glm::vec2{35.0f, 1.0f},
  glm::vec2{52.0f, 1.0f},
  glm::vec2{69.0f, 1.0f},
  
  glm::vec2{1.0f , 22.0f},
  glm::vec2{18.0f, 22.0f},
  glm::vec2{35.0f, 22.0f},
  glm::vec2{52.0f, 22.0f},
  glm::vec2{69.0f, 22.0f}
};
// clang format on

// In pixels
static const auto g_number_texture_size{glm::vec2{16.0f, 20.0f}};

static const auto g_score_box_origin{glm::vec2{362.5f, 28.0f}};
//static const auto g_score_box_dims{glm::vec2{55.0f, 30.0f}};

static constexpr const auto max_score_digits{std::numeric_limits<mod_2048::points_t>::max_digits10};
using score_digits_buffer_t = std::array<std::uint8_t, max_score_digits>;

void mod_2048::score::draw() noexcept {
  using namespace surge::atom;

  const auto &img_shader{get_img_shader()};
  const auto &numbers_buffer{get_numbers_buffer()};
  
  // TODO: Change to actual score digits
  const std::array<std::uint8_t, 10> digits{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  draw_t digit_data;
  digit_data.projection = get_projection();
  digit_data.view = get_view();
  digit_data.pos = glm::vec3{g_score_box_origin, 0.2};
  digit_data.scale = glm::vec3{g_number_texture_size, 1.0f};
  digit_data.region_dims = g_number_texture_size;
  digit_data.h_flip = false;
  digit_data.v_flip = false;
  
  
  for(const auto &digit : digits) {
    digit_data.region_origin = g_number_texture_origins[digit];
    static_image::draw(img_shader, numbers_buffer, digit_data);
    digit_data.pos += glm::vec3{digit_data.scale[0], 0.0f, 0.0f};
  }

}