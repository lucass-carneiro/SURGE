#ifndef DTU_UI_CHARACTER_SHEET_HPP
#define DTU_UI_CHARACTER_SHEET_HPP

// clang-format off
#include "DTU.hpp"
#include "commands.hpp"
// clang-format on

namespace DTU::ui::character_sheet {

void load(vec_glui &ids, vec_glui64 &handles) noexcept;

/*void update(cmdq_t &cmdq, const sbd_t &ui_sbd, sdl_t &ui_sdl, tdd_t &tdd, tgl_t &tgd,
            const glm::vec2 &window_dims, const glm::vec2 &cursor_pos) noexcept;*/
void update(GLFWwindow *window, const sbd_t &ui_sbd, sdl_t &sdl) noexcept;

void mouse_left_click(cmdq_t &cmdq, const glm::vec2 &cursor_pos) noexcept;
void mouse_scroll_up(cmdq_t &cmdq, const glm::vec2 &cursor_pos) noexcept;
void mouse_scroll_down(cmdq_t &cmdq, const glm::vec2 &cursor_pos) noexcept;

} // namespace DTU::ui::character_sheet

#endif // DTU_UI_CHARACTER_SHEET_HPP