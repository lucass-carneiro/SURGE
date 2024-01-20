#ifndef SURGE_DTU_STATE_MAIN_MENU
#define SURGE_DTU_STATE_MAIN_MENU

#include "player/container_types.hpp"

#include <glm/glm.hpp>

namespace DTU::state::main_menu {

struct shader_indices {
  unsigned int nonuniform_tiles;
  unsigned int static_image;
};

enum commands : surge::u32 { idle, show_title, shift_title, show_menu };

auto load(surge::queue<surge::u32> &cmdq, float ww, float wh) noexcept -> int;
auto unload(surge::queue<surge::u32> &cmdq) noexcept -> int;

auto draw(const shader_indices &&si, glm::mat4 &proj, glm::mat4 &view) noexcept -> int;
auto update(surge::queue<surge::u32> &cmdq, double dt) noexcept -> int;

void keyboard_event(surge::queue<surge::u32> &cmdq, int key, int scancode, int action,
                    int mods) noexcept;

} // namespace DTU::state::main_menu

#endif // SURGE_DTU_STATE_MAIN_MENU