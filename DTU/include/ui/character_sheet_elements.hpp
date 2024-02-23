#ifndef DTU_CHARACTER_SHEET_MARKERS_HPP
#define DTU_CHARACTER_SHEET_MARKERS_HPP

#include "ui.hpp"

namespace DTU::ui::character_sheet {

namespace color {
static constexpr glm::vec4 white{1.0f, 1.0f, 1.0f, 1.0f};
}

namespace geometry {

/*
 * Design resolution
 */
static constexpr float dw{1920};
static constexpr float dh{1080};

/*
 * Text Baselines
 */
static constexpr auto e_tbl{glm::vec3{323.257f, 114.533f + 28.547f, 0.1f}};
static constexpr auto i_tbl{glm::vec3{323.257f, 187.078f + 28.547f, 0.1f}};
static constexpr auto r_tbl{glm::vec3{172.285f, 219.538f, 0.1f}};
static constexpr auto l_tbl{glm::vec3{426.556f, 219.538f, 0.1f}};
static constexpr auto f_tbl{glm::vec3{172.285f, 282.518f, 0.1f}};
static constexpr auto a_tbl{glm::vec3{426.556f, 282.518, 0.1f}};

static constexpr auto attr_pts_tbl{glm::vec3{363.968f, 350.0f, 0.1f}};
static constexpr auto help_txt_tbl{glm::vec3{542.092f, 569.588f, 0.1f}};

/*
 * Rects
 */

static constexpr glm::vec4 e_rect{129.365f, 113.420, 246.056, 38.228};
static constexpr glm::vec4 e_bttn_rect{212.642, 152.916f, 16.143, 36.322};
static constexpr glm::vec4 e_bttn_up_rect{212.642, 152.916f, 16.143, 36.322 / 2.0};
static constexpr glm::vec4 e_bttn_down_rect{212.642, 152.916f + 36.322 / 2.0, 16.143, 36.322 / 2.0};

static constexpr glm::vec4 i_rect{233.784, 152.916, 249.272, 36.322};
static constexpr glm::vec4 i_bttn_rect{466.913, 152.916f, 16.143, 36.322};
static constexpr glm::vec4 i_bttn_up_rect{466.913, 152.916f, 16.143, 36.322 / 2.0};
static constexpr glm::vec4 i_bttn_down_rect{466.913, 152.916f + 36.322 / 2.0, 16.143, 36.322 / 2.0};

static constexpr glm::vec4 r_rect{18, 219.538, 210.785, 36.322};
static constexpr glm::vec4 r_bttn_rect{212.642, 219.538, 16.143, 36.322};
static constexpr glm::vec4 r_bttn_up_rect{212.642, 219.538, 16.143, 36.322 / 2.0};
static constexpr glm::vec4 r_bttn_down_rect{212.642, 219.538 + 36.322 / 2.0, 16.143, 36.322 / 2.0};

static constexpr glm::vec4 l_rect{94.124f, 187.077f, 281.567f, 38.351};
static constexpr glm::vec4 l_bttn_rect{466.913, 219.538, 16.143, 36.322};
static constexpr glm::vec4 l_bttn_up_rect{466.913, 219.538, 16.143, 36.322 / 2.0};
static constexpr glm::vec4 l_bttn_down_rect{466.913, 219.538 + 36.322 / 2.0, 16.143, 36.322 / 2.0};

static constexpr glm::vec4 f_rect{43.026, 282.518, 185.759, 32.321};
static constexpr glm::vec4 f_bttn_rect{212.642, 282.518, 16.143, 32.322};
static constexpr glm::vec4 f_bttn_up_rect{212.642, 282.518, 16.143, 32.322 / 2.0};
static constexpr glm::vec4 f_bttn_down_rect{212.642, 282.518 + 32.322 / 2.0, 16.143, 32.322 / 2.0};

static constexpr glm::vec4 a_rect{286.819, 285.518, 196.237, 36.321};
static constexpr glm::vec4 a_bttn_rect{466.913, 282.518, 16.143, 36.322};
static constexpr glm::vec4 a_bttn_up_rect{466.913, 282.518, 16.143, 36.322 / 2.0};
static constexpr glm::vec4 a_bttn_down_rect{466.913, 282.518 + 36.322 / 2.0, 16.143, 36.322 / 2.0};

static constexpr glm::vec4 reset_bttn_rect{128.153, 698, 86.063, 57.376};

static constexpr glm::vec4 health_points_area{128.153, 525.604, 243.695, 36.321};
static constexpr auto health_points_value{glm::vec3{335.526, 525.604, 0.1}};

static constexpr glm::vec4 actions_points_area{128.153, 567.701, 243.695, 36.321};
static constexpr auto action_points_value{glm::vec3{335.526, 576.701, 0.1}};

static constexpr glm::vec4 psyche_points_area{128.153, 612.973, 243.695, 36.321};
static constexpr auto psyche_points_value{glm::vec3{335.526, 612.973, 0.1}};

static constexpr glm::vec4 initiative_points_area{128.153, 658.260, 243.695, 36.321};
static constexpr auto initiative_points_value{glm::vec3{335.526, 658.260, 0.1}};

} // namespace geometry

struct state {
  glm::vec3 scale{1.0f};

  // Attributes
  DTU::ui::u8_text empathy{0, geometry::e_tbl, color::white};
  DTU::ui::u8_text introspection{0, geometry::i_tbl, color::white};
  DTU::ui::u8_text reasoning{0, geometry::r_tbl, color::white};
  DTU::ui::u8_text linguistics{0, geometry::l_tbl, color::white};
  DTU::ui::u8_text fitness{0, geometry::f_tbl, color::white};
  DTU::ui::u8_text agility{0, geometry::a_tbl, color::white};
  DTU::ui::u8_text points{12, geometry::attr_pts_tbl, color::white};

  DTU::ui::u8_text hp{2, geometry::health_points_value, color::white};
  DTU::ui::u8_text ap{2, geometry::action_points_value, color::white};
  DTU::ui::u8_text pp{2, geometry::psyche_points_value, color::white};
  DTU::ui::u8_text in{0, geometry::initiative_points_value, color::white};

  // Help text
  DTU::ui::text help{"", geometry::help_txt_tbl, color::white};
};

} // namespace DTU::ui::character_sheet

#endif // DTU_CHARACTER_SHEET_MARKERS_HPP