#ifndef DTU_NEW_GAME_UI_POSITIONAL_DATA_HPP
#define DTU_NEW_GAME_UI_POSITIONAL_DATA_HPP

#include <glm/glm.hpp>

// UI positional data

namespace help {
constexpr const glm::vec4 text_area_rect{542.092f, 569.588f, 451.815f, 176.365f};
} // namespace help

namespace empathy {

constexpr const glm::vec4 group_rect{29.466f, 152.916f, 199.319f, 36.322f};
constexpr const glm::vec2 rank_baseline{172.285f, 152.916f + 28.0f};
constexpr const glm::vec4 button_rect{212.642f, 152.916f, 16.143f, 36.322f};

constexpr const glm::vec4 up_rect{button_rect[0], button_rect[1], button_rect[2],
                                  button_rect[3] / 2.0f};
constexpr const glm::vec4 down_rect{button_rect[0], button_rect[1] + button_rect[3] / 2.0f,
                                    button_rect[2], button_rect[3] / 2.0f};
} // namespace empathy

namespace introspection {

constexpr const glm::vec4 group_rect{233.784f, 152.916f, 249.272f, 36.321f};

} // namespace introspection

namespace remaining_points {
constexpr const glm::vec2 value_baseline{363.968f, 378.0f};
}

#endif // DTU_NEW_GAME_UI_POSITIONAL_DATA_HPP