#ifndef SURGE_MODULE_DTU_ERROR_TYPES_HPP
#define SURGE_MODULE_DTU_ERROR_TYPES_HPP

#include "integer_types.hpp"

namespace DTU {

enum class error : u32 {
  // Binding and unbinding callbacks
  keyboard_event_binding = 1,
  keyboard_event_unbinding,

  mouse_button_event_binding,
  mouse_button_event_unbinding,

  mouse_scroll_event_binding,
  mouse_scroll_event_unbinding,
};

} // namespace DTU

#endif // SURGE_MODULE_DTU_ERROR_TYPES_HPP