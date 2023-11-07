#include "debug_window.hpp"

#include "logging.hpp"
#include "pieces.hpp"

void mod_2048::debug_window::init(GLFWwindow *window) noexcept {
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

void mod_2048::debug_window::draw() noexcept {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  main_window();

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void mod_2048::debug_window::main_window(bool *p_open) noexcept {
  using namespace mod_2048;
  using namespace mod_2048::pieces;

  // We specify a default position/size in case there's no data in the .ini file.
  ImGui::SetNextWindowPos(ImVec2(0.0, 0.0), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_FirstUseEver);

  ImGuiWindowFlags window_flags{ImGuiWindowFlags_NoResize};
  ImGuiTableFlags table_flags{ImGuiTableFlags_Borders};

  if (!ImGui::Begin("2048 debug window", p_open, window_flags)) {
    // Early out if the window is collapsed, as an optimization.
    ImGui::End();
    return;
  }

  static pieces::slot_t slot_input{0};
  static pieces::exponent_t exponent_input{1};
  const pieces::slot_t step_input{1};

  if (ImGui::CollapsingHeader("Add Pieces")) {
    ImGui::SeparatorText("Random");

    if (ImGui::Button("Add")) {
      pieces::create_random();
    }

    ImGui::SeparatorText("Precise");

    ImGui::InputScalar("Slot", ImGuiDataType_U8, &slot_input, &step_input, nullptr, "%u");
    ImGui::InputScalar("Exponent", ImGuiDataType_U8, &exponent_input, &step_input, nullptr, "%u");

    if (ImGui::Button("Add piece")) {
      // pieces::add(slot_input, exponent_input);
    }
  }

  const auto &positions{pieces::get_piece_positions()};
  const auto &exponents{pieces::get_piece_exponents()};
  const auto &slots{pieces::get_piece_slots()};

  if (ImGui::CollapsingHeader("Piece Data")) {
    if (ImGui::BeginTable("pieces_table", 5, table_flags)) {
      ImGui::TableSetupColumn("Piece ID");
      ImGui::TableSetupColumn("x");
      ImGui::TableSetupColumn("y");
      ImGui::TableSetupColumn("Exp");
      ImGui::TableSetupColumn("Slot");
      ImGui::TableHeadersRow();

      for (const auto &p : positions) {
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::Text("%d", p.first);

        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%f", p.second[0]);

        ImGui::TableSetColumnIndex(2);
        ImGui::Text("%f", p.second[1]);

        ImGui::TableSetColumnIndex(3);
        ImGui::Text("%d", exponents.at(p.first));

        ImGui::TableSetColumnIndex(4);
        ImGui::Text("%d", slots.at(p.first));
      }

      ImGui::EndTable();
    }
  }

  ImGui::End();
}