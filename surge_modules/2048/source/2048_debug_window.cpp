#include "2048_debug_window.hpp"

#include "2048_pieces.hpp"

#include <imgui.h>

void mod_2048::debug_window::draw() noexcept {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  main_window();

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void mod_2048::debug_window::main_window(bool *p_open) noexcept {
  // We specify a default position/size in case there's no data in the .ini file.
  ImGui::SetNextWindowPos(ImVec2(0.0, 0.0), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_FirstUseEver);

  ImGuiWindowFlags window_flags{ImGuiWindowFlags_NoResize};
  ImGuiTableFlags table_flags{ImGuiTableFlags_Borders};

  const auto &poss{pieces::get_positions()};
  const auto &exps{pieces::get_exponents()};
  const auto &slts{pieces::get_slots()};
  const auto &ocp{pieces::get_occupation()};

  static pieces::u8 slot_input{0};
  static pieces::u8 exponent_input{1};
  const pieces::u8 step_input{1};

  if (!ImGui::Begin("2048 debug window", p_open, window_flags)) {
    // Early out if the window is collapsed, as an optimization.
    ImGui::End();
    return;
  }

  if (ImGui::CollapsingHeader("Piece control")) {
    ImGui::SeparatorText("Piece creation");
    if (ImGui::Button("Add Random")) {
      pieces::add_random();
    }

    ImGui::InputScalar("Slot", ImGuiDataType_U8, &slot_input, &step_input, nullptr, "%u");
    ImGui::InputScalar("Exponent", ImGuiDataType_U8, &exponent_input, &step_input, nullptr, "%u");

    if (ImGui::Button("Add piece")) {
      pieces::add(slot_input, exponent_input);
    }
  }

  if (ImGui::CollapsingHeader("Pieces and Occupation data")) {

    ImGui::SeparatorText("Pieces table");

    if (ImGui::BeginTable("pieces_table", 5, table_flags)) {
      ImGui::TableSetupColumn("Piece ID");
      ImGui::TableSetupColumn("x");
      ImGui::TableSetupColumn("y");
      ImGui::TableSetupColumn("Exp");
      ImGui::TableSetupColumn("Slot");
      ImGui::TableHeadersRow();

      for (int i = 0; i < 16; i++) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("%d", i);

        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%.1f", poss.x[i]);

        ImGui::TableSetColumnIndex(2);
        ImGui::Text("%.1f", poss.y[i]);

        ImGui::TableSetColumnIndex(3);
        ImGui::Text("%u", exps[i]);

        ImGui::TableSetColumnIndex(4);
        ImGui::Text("%u", slts[i]);
      }

      ImGui::EndTable();
    }

    ImGui::SeparatorText("Occupation table");

    if (ImGui::BeginTable("occupation_table", 2, table_flags)) {
      ImGui::TableSetupColumn("Slot");
      ImGui::TableSetupColumn("Piece ID");
      ImGui::TableHeadersRow();

      for (int i = 0; i < 16; i++) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("%d", i);

        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%u", ocp[i]);
      }

      ImGui::EndTable();
    }
  }

  if (ImGui::CollapsingHeader("Board representation")) {
  }

  ImGui::End();
}