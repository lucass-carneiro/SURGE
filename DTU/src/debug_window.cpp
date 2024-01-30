#include "debug_window.hpp"

#include "player/logging.hpp"
#include "states.hpp"

void DTU::debug_window::init(GLFWwindow *window) noexcept {
  log_info("Initializing debug window");
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  /* ImGuiIO &io =  ImGui::GetIO();
  (void)io;
   io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
   io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
  */

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  // ImGui::StyleColorsLight();

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, false);
  ImGui_ImplOpenGL3_Init("#version 460");
}

void DTU::debug_window::draw() noexcept {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  main_window();

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void DTU::debug_window::cleanup() noexcept {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void DTU::debug_window::main_window(bool *p_open) noexcept {
  // We specify a default position/size in case there's no data in the .ini file.
  ImGui::SetNextWindowPos(ImVec2(0.0, 0.0), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_FirstUseEver);

  ImGuiWindowFlags window_flags{ImGuiWindowFlags_NoResize};
  ImGuiTableFlags table_flags{ImGuiTableFlags_Borders};

  if (!ImGui::Begin("DTU debug window", p_open, window_flags)) {
    // Early out if the window is collapsed, as an optimization.
    ImGui::End();
    return;
  }

  if (ImGui::CollapsingHeader("DTU State machine")) {
    if (ImGui::BeginTable("state_machine_table", 3, table_flags)) {
      ImGui::TableSetupColumn("State variable");
      ImGui::TableSetupColumn("ID");
      ImGui::TableSetupColumn("Desc.");
      ImGui::TableHeadersRow();

      const auto state_a{DTU::state_machine::get_a()};
      const auto state_b{DTU::state_machine::get_b()};

      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::Text("%s", "A");

      ImGui::TableSetColumnIndex(1);
      ImGui::Text("%u", state_a);

      ImGui::TableSetColumnIndex(2);
      ImGui::Text("%s", DTU::state_machine::to_str(state_a));

      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::Text("%s", "B");

      ImGui::TableSetColumnIndex(1);
      ImGui::Text("%u", state_b);

      ImGui::TableSetColumnIndex(2);
      ImGui::Text("%s", DTU::state_machine::to_str(state_b));

      ImGui::EndTable();
    }
  }

  ImGui::End();
}