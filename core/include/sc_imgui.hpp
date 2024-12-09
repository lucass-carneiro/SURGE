#ifndef SURGE_CORE_SC_IMGUI_HPP
#define SURGE_CORE_SC_IMGUI_HPP

#include "sc_glfw_includes.hpp"

#include <imgui.h>

namespace surge::imgui {

enum class themes { light, dark };

struct create_config {
  bool keyboard_nav{false};
  bool gamepad_nav{false};
  themes theme{themes::dark};
};

void mouse_callback(window::window_t w, int button, int action, int mods) noexcept;
void mouse_scroll_callback(window::window_t w, double xoffset, double yoffset) noexcept;

} // namespace surge::imgui

#endif // SURGE_CORE_SC_IMGUI_HPP