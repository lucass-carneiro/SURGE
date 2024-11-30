#include "imgui_demo.hpp"

#include "sc_opengl/atoms/imgui.hpp"

static ImGuiContext *imgui_ctx{nullptr};

extern "C" SURGE_MODULE_EXPORT auto on_load() -> int {
  using namespace surge;

  imgui_ctx = gl_atom::imgui::create(imgui::create_config{});
  ImGui::SetCurrentContext(imgui_ctx);

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto on_unload() -> int {
  using namespace surge;
  gl_atom::imgui::destroy(imgui_ctx);
  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto draw() -> int {
  using namespace surge;

  gl_atom::imgui::frame_begin();

  ImGui::SetWindowPos({0.0f, 0.0f});
  ImGui::ShowDemoWindow();

  gl_atom::imgui::frame_end();

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto update(double) -> int { return 0; }

extern "C" SURGE_MODULE_EXPORT void keyboard_event(int, int, int, int) {}

extern "C" SURGE_MODULE_EXPORT void mouse_button_event(int button, int action, int mods) {
  using namespace surge;
  surge::imgui::register_mouse_callback(button, action, mods);
}

extern "C" SURGE_MODULE_EXPORT void mouse_scroll_event(double xoffset, double yoffset) {
  using namespace surge;
  surge::imgui::register_mouse_scroll_callback(xoffset, yoffset);
}