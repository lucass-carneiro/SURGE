#include "imgui_demo.hpp"

#include "sc_opengl/atoms/imgui.hpp"

static ImGuiContext *imgui_ctx{nullptr};

extern "C" SURGE_MODULE_EXPORT auto gl_on_load(surge::window::window_t w) -> int {
  using namespace surge;

  // Create ImGui context
  imgui_ctx = gl_atom::imgui::create(w, imgui::create_config{});
  ImGui::SetCurrentContext(imgui_ctx);

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto gl_on_unload(surge::window::window_t) -> int {
  using namespace surge;
  gl_atom::imgui::destroy(imgui_ctx);
  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto gl_draw(surge::window::window_t) -> int {
  using namespace surge;

  gl_atom::imgui::frame_begin();

  ImGui::SetWindowPos({0.0f, 0.0f});
  ImGui::ShowDemoWindow();

  gl_atom::imgui::frame_end();

  return 0;
}

extern "C" SURGE_MODULE_EXPORT auto gl_update(surge::window::window_t, double) -> int { return 0; }

extern "C" SURGE_MODULE_EXPORT void gl_keyboard_event(surge::window::window_t, int, int, int, int) {
}

extern "C" SURGE_MODULE_EXPORT void gl_mouse_button_event(surge::window::window_t w, int button,
                                                          int action, int mods) {
  using namespace surge;
  surge::imgui::mouse_callback(w, button, action, mods);
}

extern "C" SURGE_MODULE_EXPORT void gl_mouse_scroll_event(surge::window::window_t w, double xoffset,
                                                          double yoffset) {
  using namespace surge;
  surge::imgui::mouse_scroll_callback(w, xoffset, yoffset);
}