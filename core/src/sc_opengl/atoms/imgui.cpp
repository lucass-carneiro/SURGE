#include "sc_opengl/atoms/imgui.hpp"

#include "sc_window.hpp"

// clang-format off
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
// clang-format on

auto surge::gl_atom::imgui::create(window::window_t w, surge::imgui::create_config &&cfg)
    -> ImGuiContext * {
  using namespace surge::imgui;

  log_info("Initializing DearImGui window");

  IMGUI_CHECKVERSION();
  auto ctx{ImGui::CreateContext()};

  // Nav
  ImGuiIO &io = ImGui::GetIO();

  if (cfg.keyboard_nav) {
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  }

  if (cfg.gamepad_nav) {
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
  }

  // Style
  if (cfg.theme == themes::dark) {
    ImGui::StyleColorsDark();
  } else {
    ImGui::StyleColorsLight();
  }

  // Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(w, false);
  ImGui_ImplOpenGL3_Init("#version 460");

  return ctx;
}

void surge::gl_atom::imgui::destroy(ImGuiContext *ctx) {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext(ctx);
}

void surge::gl_atom::imgui::frame_begin() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void surge::gl_atom::imgui::frame_end() {
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

#ifdef SURGE_BUILD_TYPE_Debug

void surge::gl_atom::imgui::texture_database_window(bool *open,
                                                    const gl_atom::texture::database &tdb) {
  // Early out if the window is collapsed, as an optimization.
  if (!ImGui::Begin("Texture Database", open)) {
    ImGui::End();
    return;
  }

  if (ImGui::BeginTable("tdb_table", 4)) {
    const auto &ids{tdb.get_ids()};
    const auto &handles{tdb.get_handles()};
    const auto &hashes{tdb.get_hashes()};

    ImGui::TableSetupColumn("Element");
    ImGui::TableSetupColumn("Id");
    ImGui::TableSetupColumn("Handle");
    ImGui::TableSetupColumn("Hash");
    ImGui::TableHeadersRow();

    for (surge::usize i = 0; i < tdb.size(); i++) {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();

      ImGui::Text("%lu", i);
      ImGui::TableNextColumn();

      ImGui::Text("%u", ids[i]);
      ImGui::TableNextColumn();

      ImGui::Text("%lu", handles[i]);
      ImGui::TableNextColumn();

      ImGui::Text("%lu", hashes[i]);
    }

    ImGui::EndTable();
  }

  ImGui::End();
}

void surge::gl_atom::imgui::sprite_database_window(bool *open,
                                                   const gl_atom::sprite_database::database sdb) {
  // Early out if the window is collapsed, as an optimization.
  if (!ImGui::Begin("Sprite Database", open, 0)) {
    ImGui::End();
    return;
  }

  constexpr ImGuiTableFlags flags{ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersV
                                  | ImGuiTableFlags_BordersH
                                  | ImGuiTableFlags_HighlightHoveredColumn};

  if (ImGui::BeginTable("sdb_table", 2, flags)) {
    ImGui::TableSetupColumn("Buffer Index");
    ImGui::TableSetupColumn("Elements in buffer");
    ImGui::TableHeadersRow();

    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    ImGui::Text("%lu", gl_atom::sprite_database::get_current_buffer_idx(sdb));
    ImGui::TableNextColumn();

    ImGui::Text("%lu", gl_atom::sprite_database::get_sprites_in_buffer(sdb));

    ImGui::EndTable();
  }

  ImGui::End();
}

#endif // SURGE_BUILD_TYPE_Debug