#include "debug_window.hpp"

#include "2048.hpp"
#include "logging.hpp"
#include "pieces.hpp"

#include <imgui.h>

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

void mod_2048::debug_window::cleanup() noexcept {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

static auto get_state_string(const mod_2048::game_state &state) -> const char * {
  using namespace mod_2048;
  switch (static_cast<state_code_t>(state)) {
  case static_cast<state_code_t>(game_state::idle):
    return "Idle";
  case static_cast<state_code_t>(game_state::compress_up):
    return "Compress up";
  case static_cast<state_code_t>(game_state::compress_down):
    return "Compress down";
  case static_cast<state_code_t>(game_state::compress_left):
    return "Compress left";
  case static_cast<state_code_t>(game_state::compress_right):
    return "Compress right";
  case static_cast<state_code_t>(game_state::merge_up):
    return "Merge up";
  case static_cast<state_code_t>(game_state::merge_down):
    return "Merge down";
  case static_cast<state_code_t>(game_state::merge_left):
    return "Merge left";
  case static_cast<state_code_t>(game_state::merge_right):
    return "Merge right";
  case static_cast<state_code_t>(game_state::piece_removal):
    return "Piece removal";
  case static_cast<state_code_t>(game_state::add_piece):
    return "Add piece";
  default:
    return "Unknown state";
  }
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

  const auto &positions{pieces::get_piece_positions()};
  const auto &exponents{pieces::get_piece_exponents()};
  const auto &slots{pieces::get_piece_slots()};
  auto &target_slots{pieces::get_piece_target_slots()};
  const auto &state_stack{view_state_queue()};

  static pieces::slot_t slot_input{0};
  static pieces::exponent_t exponent_input{1};
  static pieces::piece_id_t piece_id_input{0};
  const pieces::slot_t step_input{1};

  if (ImGui::CollapsingHeader("Add random")) {
    if (ImGui::Button("Add")) {
      pieces::create_random();
    }
  }

  if (ImGui::CollapsingHeader("Add Precise")) {
    ImGui::InputScalar("Slot", ImGuiDataType_U8, &slot_input, &step_input, nullptr, "%u");
    ImGui::InputScalar("Exponent", ImGuiDataType_U8, &exponent_input, &step_input, nullptr, "%u");

    if (ImGui::Button("Add piece")) {
      pieces::create_piece(exponent_input, slot_input);
    }
  }

  if (ImGui::CollapsingHeader("Remove Precise")) {
    ImGui::InputScalar("Piece ID", ImGuiDataType_U8, &piece_id_input, &step_input, nullptr, "%u");

    if (ImGui::Button("Remove piece")) {
      pieces::delete_piece(piece_id_input);
    }
  }

  if (ImGui::CollapsingHeader("Set Target Slot")) {
    ImGui::InputScalar("Piece ID", ImGuiDataType_U8, &piece_id_input, &step_input, nullptr, "%u");
    ImGui::InputScalar("Slot", ImGuiDataType_U8, &slot_input, &step_input, nullptr, "%u");

    if (ImGui::Button("Set target")) {
      target_slots.at(piece_id_input) = slot_input;
    }
  }

  if (ImGui::CollapsingHeader("Piece Data")) {
    if (ImGui::BeginTable("pieces_table", 6, table_flags)) {
      ImGui::TableSetupColumn("ID");
      ImGui::TableSetupColumn("x");
      ImGui::TableSetupColumn("y");
      ImGui::TableSetupColumn("Exp");
      ImGui::TableSetupColumn("Slot");
      ImGui::TableSetupColumn("Tgt. Slot");
      ImGui::TableHeadersRow();

      for (const auto &p : positions) {
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::Text("%d", p.first);

        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%.4f", p.second[0]);

        ImGui::TableSetColumnIndex(2);
        ImGui::Text("%.4f", p.second[1]);

        ImGui::TableSetColumnIndex(3);
        ImGui::Text("%d", exponents.at(p.first));

        ImGui::TableSetColumnIndex(4);
        ImGui::Text("%d", slots.at(p.first));

        ImGui::TableSetColumnIndex(5);
        ImGui::Text("%d", target_slots.at(p.first));
      }

      ImGui::EndTable();
    }
  }

  if (ImGui::CollapsingHeader("State stack data")) {
    if (ImGui::BeginTable("game_state_table", 3, table_flags)) {
      ImGui::TableSetupColumn("Element");
      ImGui::TableSetupColumn("State Code");
      ImGui::TableSetupColumn("State desc.");
      ImGui::TableHeadersRow();

      for (std::size_t i = 0; const auto &s : state_stack) {
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::Text("%lu", i);

        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%u", static_cast<state_code_t>(s));

        ImGui::TableSetColumnIndex(2);
        ImGui::Text("%s", get_state_string(s));
        i++;
      }

      ImGui::EndTable();
    }
  }

  ImGui::End();
}