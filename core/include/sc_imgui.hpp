#ifndef SURGE_CORE_SC_IMGUI_HPP
#define SURGE_CORE_SC_IMGUI_HPP

#include <imgui.h>

namespace surge::imgui {

enum class themes { light, dark };

struct create_config {
  bool keyboard_nav{false};
  bool gamepad_nav{false};
  themes theme{themes::dark};
};

void register_mouse_callback(int button, int action, int mods) noexcept;
void register_mouse_scroll_callback(double xoffset, double yoffset) noexcept;

} // namespace surge::imgui

#endif // SURGE_CORE_SC_IMGUI_HPP