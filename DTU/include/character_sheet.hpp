#ifndef DTU_UI_CHARACTER_SHEET_HPP
#define DTU_UI_CHARACTER_SHEET_HPP

// clang-format off
#include "DTU.hpp"
#include "commands.hpp"
// clang-format on

namespace DTU::ui::character_sheet {

void load(vec_glui &ids, vec_glui64 &handles) noexcept;

void update(GLFWwindow *window, sdl_t &ui_sdl, tdd_t &tdd, tgd_t &tgd) noexcept;

} // namespace DTU::ui::character_sheet

#endif // DTU_UI_CHARACTER_SHEET_HPP