#ifndef DTU_COMMANDS_HPP
#define DTU_COMMANDS_HPP

#include <player/container_types.hpp>

namespace DTU {

using cmd_code_t = surge::u32;
using cmdq_t = surge::deque<cmd_code_t>;

enum commands : cmd_code_t {
  idle,

  show_title,
  show_menu,
  shift_opt_left,
  shift_opt_right,
  enter_option,

  empathy_up,
  empathy_down,

  introspection_up,
  introspection_down,

  linguistics_up,
  linguistics_down,

  reasoning_up,
  reasoning_down,

  fitness_up,
  fitness_down,

  agility_up,
  agility_down,

  ui_refresh,

  count
};

} // namespace DTU

#endif // DTU_COMMANDS_HPP