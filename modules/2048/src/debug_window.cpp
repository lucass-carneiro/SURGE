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

void mod_2048::debug_window::cleanup() noexcept {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
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
  auto &command_queue{pieces::get_piece_command_queue()};

  static pieces::slot_t slot_input{0};
  static pieces::exponent_t exponent_input{1};
  static pieces::piece_id_t piece_id_input{0};
  static pieces::piece_command_code_t command_code_input{0};
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

  if (ImGui::CollapsingHeader("Piece commands")) {
    ImGui::InputScalar("Piece ID", ImGuiDataType_U8, &piece_id_input, &step_input, nullptr, "%u");
    ImGui::InputScalar("Command", ImGuiDataType_U8, &command_code_input, &step_input, nullptr,
                       "%u");

    if (ImGui::Button("Push command")) {
      const pieces::command_t packed_command = (piece_id_input << 4) | command_code_input;
      command_queue.push_back(packed_command);
    }

    if (ImGui::BeginTable("pieces_command_queue_table", 3, table_flags)) {
      ImGui::TableSetupColumn("ID");
      ImGui::TableSetupColumn("Command");
      ImGui::TableSetupColumn("Description");
      ImGui::TableHeadersRow();

      for (const auto &pc : command_queue) {
        const auto id{(pc >> 4) & 0x0F};
        const auto command{pc & 0x0F};

        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::Text("%d", id);

        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%d", command);

        ImGui::TableSetColumnIndex(2);
        ImGui::Text("%s", get_command_desc(static_cast<commands>(command)));
      }

      ImGui::EndTable();
    }
  }

  ImGui::End();
}