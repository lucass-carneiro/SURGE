#include "debug_window.hpp"

#include "DTU.hpp"
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

void DTU::debug_window::draw(const DTU::vec_glui &ids, const vec_glui64 &handles,
                             const cmdq_t &cmdq, const sdl_t &sdl, const sdl_t &ui_sdl) noexcept {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  main_window(ids, handles, cmdq, sdl, ui_sdl);

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void DTU::debug_window::cleanup() noexcept {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void DTU::debug_window::main_window(const DTU::vec_glui &ids, const vec_glui64 &handles,
                                    const cmdq_t &cmdq, const sdl_t &sdl, const sdl_t &ui_sdl,
                                    bool *p_open) noexcept {

  // We specify a default position/size in case there's no data in the .ini file.
  ImGui::SetNextWindowPos(ImVec2(0.0, 0.0), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(800, 400), ImGuiCond_FirstUseEver);

  ImGuiWindowFlags window_flags{ImGuiWindowFlags_NoResize};
  ImGuiTableFlags table_flags{ImGuiTableFlags_Borders};

  if (!ImGui::Begin("DTU debug window", p_open, window_flags)) {
    // Early out if the window is collapsed, as an optimization.
    ImGui::End();
    return;
  }

  if (ImGui::CollapsingHeader("State machine")) {
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

    // State transitions
    static surge::i32 target_state{0};

    if (ImGui::Button("Transition")) {
      if (target_state > 0 && target_state < surge::i32{DTU::state_machine::states::count}) {
        DTU::state_machine::push_state(static_cast<surge::u32>(target_state));
        target_state = 0;
      }
    }
    ImGui::SameLine();
    ImGui::InputInt("Target State", &target_state);
  }

  if (ImGui::CollapsingHeader("Command queue")) {
    if (ImGui::BeginTable("command_queue_table", 2, table_flags)) {
      ImGui::TableSetupColumn("Index");
      ImGui::TableSetupColumn("Command ID");
      ImGui::TableHeadersRow();

      for (surge::usize i = 0; const auto &cmd : cmdq) {
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::Text("%lu", i);

        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%u", cmd);

        i++;
      }

      ImGui::EndTable();
    }
  }

  if (ImGui::CollapsingHeader("Sprite Textures")) {
    if (ImGui::BeginTable("sprite_texture_table", 4, table_flags)) {
      ImGui::TableSetupColumn("ID");
      ImGui::TableSetupColumn("Texture ID");
      ImGui::TableSetupColumn("Texture Handle");
      ImGui::TableSetupColumn("Resident");
      ImGui::TableHeadersRow();

      for (surge::usize i = 0; i < ids.size(); i++) {
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::Text("%lu", i);

        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%u", ids[i]);

        ImGui::TableSetColumnIndex(2);
        ImGui::Text("%lu", handles[i]);

        ImGui::TableSetColumnIndex(3);
        ImGui::Text("%s", surge::atom::sprite::is_resident(handles[i]) ? "Yes" : "No");
      }

      ImGui::EndTable();
    }
  }

  if (ImGui::CollapsingHeader("Sprite draw data")) {
    if (ImGui::BeginTable("sprite_draw_data_table", 6, table_flags)) {
      ImGui::TableSetupColumn("Idx.");
      ImGui::TableSetupColumn("TH");
      ImGui::TableSetupColumn("TA");
      ImGui::TableSetupColumn("x");
      ImGui::TableSetupColumn("y");
      ImGui::TableSetupColumn("z");
      ImGui::TableHeadersRow();

      for (surge::usize i = 0; i < sdl.alphas.size(); i++) {
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::Text("%lu", i);

        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%lu", sdl.texture_handles[i]);

        ImGui::TableSetColumnIndex(2);
        ImGui::Text("%.3f", sdl.alphas[i]);

        ImGui::TableSetColumnIndex(3);
        ImGui::Text("%.3f", sdl.models[i][3][0]);

        ImGui::TableSetColumnIndex(4);
        ImGui::Text("%.3f", sdl.models[i][3][1]);

        ImGui::TableSetColumnIndex(5);
        ImGui::Text("%.3f", sdl.models[i][3][2]);
      }

      ImGui::EndTable();
    }
  }

  if (ImGui::CollapsingHeader("UI Sprite draw data")) {
    if (ImGui::BeginTable("ui_sprite_draw_data_table", 6, table_flags)) {
      ImGui::TableSetupColumn("Idx.");
      ImGui::TableSetupColumn("TH");
      ImGui::TableSetupColumn("TA");
      ImGui::TableSetupColumn("x");
      ImGui::TableSetupColumn("y");
      ImGui::TableSetupColumn("z");
      ImGui::TableHeadersRow();

      for (surge::usize i = 0; i < ui_sdl.alphas.size(); i++) {
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        ImGui::Text("%lu", i);

        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%lu", ui_sdl.texture_handles[i]);

        ImGui::TableSetColumnIndex(2);
        ImGui::Text("%.3f", ui_sdl.alphas[i]);

        ImGui::TableSetColumnIndex(3);
        ImGui::Text("%.3f", ui_sdl.models[i][3][0]);

        ImGui::TableSetColumnIndex(4);
        ImGui::Text("%.3f", ui_sdl.models[i][3][1]);

        ImGui::TableSetColumnIndex(5);
        ImGui::Text("%.3f", ui_sdl.models[i][3][2]);
      }

      ImGui::EndTable();
    }
  }

  ImGui::End();
}