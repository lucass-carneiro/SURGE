#ifndef DTU_UI_CHARACTER_SHEET_HPP
#define DTU_UI_CHARACTER_SHEET_HPP

// clang-format off
#include "DTU.hpp"
#include "commands.hpp"

#include <player/sprite.hpp>
#include <player/text.hpp>
#include <player/window.hpp>

#include <string_view>
// clang-format on

namespace DTU::ui::character_sheet {

namespace color {
constexpr const glm::vec4 white{1.0f, 1.0f, 1.0f, 1.0f};
}

namespace geometry {

constexpr const float baseline_skip{32.0f};

constexpr const glm::vec3 empathy_text_baseline{170.285f, 152.916f + baseline_skip, 0.1f};
constexpr const glm::vec3 introspection_text_baseline{426.556f, 152.916f + baseline_skip, 0.1f};
constexpr const glm::vec3 reasoning_text_baseline{172.285f, 219.538f + baseline_skip, 0.1f};
constexpr const glm::vec3 linguistics_text_baseline{426.556f, 219.538f + baseline_skip, 0.1f};
constexpr const glm::vec3 fitness_text_baseline{172.285f, 282.518f + baseline_skip, 0.1f};
constexpr const glm::vec3 agility_text_baseline{426.556f, 282.518 + baseline_skip, 0.1f};

constexpr const glm::vec3 points_text_baseline{363.968f, 350.0f + baseline_skip, 0.1f};

constexpr const glm::vec3 help_text_baseline{542.092f, 569.588f + baseline_skip, 0.1f};

} // namespace geometry

namespace elements {

struct background {
  GLuint texture_ID{0};
  GLuint64 texture_handle{0};
  glm::vec3 scale{1.0f};
};

struct u8_text {
  surge::u8 value;
  glm::vec3 baseline{0.0f};
  glm::vec4 color{0.0f};
};

struct text {
  std::string_view text{};
  glm::vec3 baseline{0.0f};
  glm::vec4 color{0.0f};
};

} // namespace elements

struct ui_state_desc {
  elements::background background;

  // Attributes
  elements::u8_text empathy{0, geometry::empathy_text_baseline, color::white};
  elements::u8_text introspection{0, geometry::introspection_text_baseline, color::white};
  elements::u8_text reasoning{0, geometry::reasoning_text_baseline, color::white};
  elements::u8_text fitness{0, geometry::fitness_text_baseline, color::white};
  elements::u8_text agility{0, geometry::agility_text_baseline, color::white};
  elements::u8_text points{12, geometry::points_text_baseline, color::white};

  // Help text
  elements::text help{"", geometry::help_text_baseline, color::white};
};

void load(vec_glui &ids, vec_glui64 &handles, sdl_t &sdl, float ww, float wh) noexcept;

void update(GLFWwindow *window, cmdq_t &cmdq, ui_state_desc &current_state) noexcept;

void bake_and_send(const ui_state_desc &current_state, surge::atom::sprite::data_list &sdl,
                   surge::atom::text::text_draw_data &tdd,
                   surge::atom::text::glyph_data &tgd) noexcept;

} // namespace DTU::ui::character_sheet

#endif // DTU_UI_CHARACTER_SHEET_HPP