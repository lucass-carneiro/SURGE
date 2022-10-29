#include "gui_windows/gui_windows.hpp"

#include <imgui.h>

void surge::show_memory_profiler_window(bool *open) noexcept {
  using namespace ImGui;

  SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);
  if (!Begin("Memory profiler", open, ImGuiWindowFlags_NoResize)) {
    End();
    return;
  }

  End();
}