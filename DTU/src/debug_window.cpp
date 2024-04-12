#include "debug_window.hpp"

#include "player/logging.hpp"

void DTU::debug_window::create(GLFWwindow *window) noexcept {
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

void DTU::debug_window::destroy() noexcept {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void DTU::debug_window::draw(const std::tuple<float, float> &wdims, bool &show) noexcept {
  if (show) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    main_window(wdims);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  }
}

void DTU::debug_window::main_window(const std::tuple<float, float> &) noexcept {
  if (ImGui::BeginMainMenuBar()) {
    // Memory menu
    if (ImGui::BeginMenu("GPU Data")) {
      if (ImGui::MenuItem("Texture Database")) {
        // TODO
      }

      if (ImGui::MenuItem("Sprite Database")) {
        // TODO
      }

      if (ImGui::MenuItem("Text buffer 0")) {
        // TODO
      }

      if (ImGui::MenuItem("Text buffer 1")) {
        // TODO
      }

      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("States")) {
      if (ImGui::MenuItem("Status")) {
        // TODO
      }

      if (ImGui::MenuItem("Reload")) {
        // TODO
      }

      if (ImGui::MenuItem("Replace")) {
        // TODO
      }

      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Module")) {
      if (ImGui::MenuItem("Hot Reload")) {
        // TODO
      }

      if (ImGui::MenuItem("Quit")) {
        // TODO
      }

      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }
}