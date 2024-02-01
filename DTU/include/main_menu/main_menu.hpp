#ifndef SURGE_DTU_STATE_MAIN_MENU
#define SURGE_DTU_STATE_MAIN_MENU

#include "player/container_types.hpp"
#include "player/text.hpp"
#include "states.hpp"

#include <glm/glm.hpp>

namespace DTU::state::main_menu {

enum commands : surge::u32 {
  idle,
  show_title,
  show_menu,
  shift_opt_left,
  shift_opt_right,
  enter_option
};

auto load(surge::deque<surge::u32> &cmdq, surge::vector<glm::mat4> &sprite_models,
          surge::vector<GLuint64> &sprite_textures, surge::vector<float> &sprite_alphas, float ww,
          float wh) noexcept -> int;

void unload(surge::deque<surge::u32> &cmdq, surge::vector<glm::mat4> &sprite_models,
            surge::vector<GLuint64> &sprite_textures) noexcept;

void update(surge::deque<surge::u32> &cmdq, surge::vector<glm::mat4> &sprite_models,
            surge::vector<float> &sprite_alphas, double dt) noexcept;

void keyboard_event(surge::deque<surge::u32> &cmdq, int key, int scancode, int action,
                    int mods) noexcept;

} // namespace DTU::state::main_menu

#endif // SURGE_DTU_STATE_MAIN_MENU