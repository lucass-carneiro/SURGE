#include "gui_windows/gui_windows.hpp"

#include <imgui.h>

void surge::show_fps_counter_window(bool *open) noexcept {
  using namespace ImGui;

  SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);
  if (!Begin("FPS counter", open, ImGuiWindowFlags_NoResize)) {
    End();
    return;
  }

  End();
}