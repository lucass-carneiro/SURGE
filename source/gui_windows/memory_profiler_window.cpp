#include "gui_windows/gui_windows.hpp"
#include "window.hpp"

#include <imgui.h>

void surge::show_memory_profiler_window(bool *open) noexcept {
  ImGui::SetNextWindowSize(ImVec2(430, 430), ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Memory profiler", open, ImGuiWindowFlags_NoResize)) {
    ImGui::End();
  }
}