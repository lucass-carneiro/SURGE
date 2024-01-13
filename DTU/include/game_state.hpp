#ifndef SURGE_MODULE_DTU_HPP
#define SURGE_MODULE_DTU_HPP

#include "error_types.hpp"
#include "player/window.hpp"

#include <tl/expected.hpp>

namespace DTU::state_machine {

template <typename StateData> struct game_state {
  using on_load_t = tl::expected<StateData, DTU::error> (*)(GLFWwindow *);
  using on_unload_t = std::uint32_t (*)(GLFWwindow *window);
  using draw_t = std::uint32_t (*)();
  using update_t = std::uint32_t (*)(double);

  using keyboard_event_t = void (*)(GLFWwindow *, int, int, int, int);
  using mouse_button_event_t = void (*)(GLFWwindow *, int, int, int);
  using mouse_scroll_event_t = void (*)(GLFWwindow *, double, double);

  on_load_t on_load;
  on_unload_t on_unload;

  draw_t draw;
  update_t update;

  keyboard_event_t keyboard_event;
  mouse_button_event_t mouse_button_event;
  mouse_scroll_event_t mouse_scroll_event;
};

} // namespace DTU::state_machine

#endif // SURGE_MODULE_DTU_HPP