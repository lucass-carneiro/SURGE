#include "gui_windows/gui_windows.hpp"
#include "window.hpp"

void surge::show_main_gui_window(bool *open) noexcept {
  using namespace ImGui;

  // Surge apps
  static bool show_memory_profiler{false};
  static bool show_fps_counter{false};

  if (show_memory_profiler) {
    show_memory_profiler_window(&show_memory_profiler);
  }
  if (show_fps_counter) {
    show_fps_counter_window(&show_fps_counter);
  }

  // Dear ImGui Tools/Apps
  static bool show_app_metrics{false};
  static bool show_app_debug_log{false};
  static bool show_app_stack_tool{false};
  static bool show_app_about{false};
  static bool show_app_style_editor{false};

  if (show_app_metrics)
    ShowMetricsWindow(&show_app_metrics);
  if (show_app_debug_log)
    ShowDebugLogWindow(&show_app_debug_log);
  if (show_app_stack_tool)
    ShowStackToolWindow(&show_app_stack_tool);
  if (show_app_about)
    ShowAboutWindow(&show_app_about);
  if (show_app_style_editor) {
    Begin("Dear ImGui Style Editor", &show_app_style_editor);
    ShowStyleEditor();
    End();
  }

  // Default position/size in case there's no data in the .ini file.
  SetNextWindowPos(ImVec2(GetMainViewport()->WorkPos.x, GetMainViewport()->WorkPos.y),
                   ImGuiCond_FirstUseEver);

  // Begin window
  if (Begin("SURGE Debug Window", open, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoResize)) {

    // Menu bar
    if (BeginMenuBar()) {

      // SURGE debug apps
      if (BeginMenu("Apps")) {
        MenuItem("FPS counter", nullptr, &show_fps_counter);
        MenuItem("Memory profiler", nullptr, &show_memory_profiler);
        EndMenu();
      }

      // DearImGui debug apps
      if (BeginMenu("GUI")) {
        MenuItem("Metrics/Debugger", nullptr, &show_app_metrics);
        MenuItem("Debug Log", nullptr, &show_app_debug_log);
        MenuItem("Stack Tool", nullptr, &show_app_stack_tool);
        MenuItem("Style Editor", nullptr, &show_app_style_editor);
        MenuItem("About Dear ImGui", nullptr, &show_app_about);
        EndMenu();
      }

      EndMenuBar();
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    Text("Welcome to SURGE v%d.%d.%d OpenGL", SURGE_VERSION_MAJOR, SURGE_VERSION_MINOR,
         SURGE_VERSION_PATCH);
    Separator();

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    Text("This window provides debug utilities for the SURGE engine.");
    BulletText("DearImGui debug apps can be found in the GUI menu");
    BulletText("SURGE debug apps can be found in the Apps");

    End();
  }
}