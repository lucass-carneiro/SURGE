#include "debug_window.hpp"

#include "options.hpp"

#include <imgui.h>

void mod_default::debug_window::draw() noexcept {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  main_window();

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void mod_default::debug_window::main_window(bool *p_open) noexcept {
  // We specify a default position/size in case there's no data in the .ini file.
  ImGui::SetNextWindowPos(ImVec2(0.0, 0.0), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);

  ImGuiWindowFlags window_flags{ImGuiWindowFlags_NoResize};

  if (!ImGui::Begin("SURGE Quick Guide", p_open, window_flags)) {
    // Early out if the window is collapsed, as an optimization.
    ImGui::End();
    return;
  }

  if (ImGui::CollapsingHeader("About")) {
    ImGui::Text("Hello, and welcome to SURGE, the Super UnderRated Game Engine.");
    ImGui::Text("Created By Lucas T. Sanches - The Ninja Sheep");
    ImGui::Text("Version %i.%i.%i", SURGE_VERSION_MAJOR, SURGE_VERSION_MINOR, SURGE_VERSION_PATCH);
  }

  if (ImGui::CollapsingHeader("Modules")) {
    ImGui::Text("Applications in SURGE are made up of loadable \"modules\". The engine \"player\"");
    ImGui::Text("component simply loads and exectutes the instructions within each module.");
    ImGui::Text("Right now, you are seeing the \"default\" module. This module is designed as an");
    ImGui::Text("example application, showcasing some of the engine's capabilities.");
  }

  if (ImGui::CollapsingHeader("Hot Reloading Modules")) {
    ImGui::Text(
        "If compiled with the SURGE_ENABLE_HR option (enabled by default on debug builds),");
    ImGui::Text("SURGE is capable of hot reloading a module. This means that a module may be");
    ImGui::Text("changed recompiled and reloadaded without closing out of the main application.");
    ImGui::Text("This is usefull while prototyping and developing games.");
    ImGui::BulletText("Sections below are demonstrating many aspects of the library.");
  }
  ImGui::End();
}