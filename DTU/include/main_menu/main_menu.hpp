#ifndef SURGE_DTU_STATE_MAIN_MENU
#define SURGE_DTU_STATE_MAIN_MENU

#include "player/container_types.hpp"
#include "player/text.hpp"

#include <glm/glm.hpp>

namespace DTU::state::main_menu {

enum commands : surge::u32 { show_title, show_menu };

auto load(surge::queue<surge::u32> &cmdq, surge::vector<glm::mat4> &sprite_models,
          surge::vector<GLuint64> &sprite_textures, surge::vector<float> &sprite_alphas, float ww,
          float wh) noexcept -> int;

auto unload(surge::queue<surge::u32> &cmdq, surge::vector<glm::mat4> &sprite_models,
            surge::vector<GLuint64> &sprite_textures) noexcept -> int;

auto update(surge::queue<surge::u32> &cmdq, surge::vector<glm::mat4> &sprite_models,
            surge::vector<float> &sprite_alphas, double dt) noexcept -> int;

void keyboard_event(surge::queue<surge::u32> &cmdq, int key, int scancode, int action,
                    int mods) noexcept;

} // namespace DTU::state::main_menu

#endif // SURGE_DTU_STATE_MAIN_MENU