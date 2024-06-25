#include "gl_atoms/imgui.hpp"

#include "logging.hpp"
#include "window.hpp"

// clang-format off
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
// clang-format on

void surge::gl_atom::imgui::create(create_config &&cfg) noexcept {
  log_info("Initializing DearImGui window");

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

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
  ImGui_ImplGlfw_InitForOpenGL(window::get_window_ptr(), false);
  ImGui_ImplOpenGL3_Init("#version 460");
}

void surge::gl_atom::imgui::destroy() noexcept {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void surge::gl_atom::imgui::frame_begin() noexcept {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void surge::gl_atom::imgui::frame_end() noexcept {
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

auto surge::gl_atom::imgui::begin(const char *name, bool *open) noexcept -> bool {
  return ImGui::Begin(name, open);
}

void surge::gl_atom::imgui::end() noexcept { ImGui::End(); }

auto surge::gl_atom::imgui::begin_main_menu_bar() noexcept -> bool {
  return ImGui::BeginMainMenuBar();
}

void surge::gl_atom::imgui::end_main_menu_bar() noexcept { ImGui::EndMainMenuBar(); }

auto surge::gl_atom::imgui::begin_menu(const char *name) noexcept -> bool {
  return ImGui::BeginMenu(name);
}

auto surge::gl_atom::imgui::menu_item(const char *name) noexcept -> bool {
  return ImGui::MenuItem(name);
}

void surge::gl_atom::imgui::end_menu() noexcept { ImGui::EndMenu(); }

auto surge::gl_atom::imgui::begin_table(const char *name, int cols) noexcept -> bool {
  constexpr ImGuiTableFlags flags{ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersV
                                  | ImGuiTableFlags_BordersH
                                  | ImGuiTableFlags_HighlightHoveredColumn};

  return ImGui::BeginTable(name, cols, flags);
}

void surge::gl_atom::imgui::end_table() noexcept { ImGui::EndTable(); }

void surge::gl_atom::imgui::register_mouse_callback(int button, int action, int mods) noexcept {
  ImGui_ImplGlfw_MouseButtonCallback(window::get_window_ptr(), button, action, mods);
}

void surge::gl_atom::imgui::register_mouse_scroll_callback(double xoffset,
                                                           double yoffset) noexcept {
  ImGui_ImplGlfw_ScrollCallback(window::get_window_ptr(), xoffset, yoffset);
}

void surge::gl_atom::imgui::texture_database_window(bool *open,
                                                    const texture::database &tdb) noexcept {
  // Early out if the window is collapsed, as an optimization.
  if (!ImGui::Begin("Texture Database", open)) {
    ImGui::End();
    return;
  }

  if (imgui::begin_table("tdb_table", 4)) {
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
                                                   const sprite::database &sdb) noexcept {
  // Early out if the window is collapsed, as an optimization.
  if (!ImGui::Begin("Sprite Database", open, 0)) {
    ImGui::End();
    return;
  }

  constexpr ImGuiTableFlags flags{ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersV
                                  | ImGuiTableFlags_BordersH
                                  | ImGuiTableFlags_HighlightHoveredColumn};

  if (ImGui::CollapsingHeader("Texture Handles")) {
    if (ImGui::BeginTable("sdb_table_texture_handle", 2, flags)) {
      ImGui::TableSetupColumn("Size");
      ImGui::TableSetupColumn("Write Buffer");
      ImGui::TableHeadersRow();

      ImGui::TableNextRow();
      ImGui::TableNextColumn();

      ImGui::Text("%lu", sdb.texture_handles.size());
      ImGui::TableNextColumn();

      ImGui::Text("%lu", sdb.texture_handles.get_write_buffer());

      ImGui::EndTable();
    }
  }

  if (ImGui::CollapsingHeader("Model Matrices")) {
    if (ImGui::BeginTable("sdb_table_model_matrices", 2, flags)) {
      ImGui::TableSetupColumn("Size");
      ImGui::TableSetupColumn("Write Buffer");
      ImGui::TableHeadersRow();

      ImGui::TableNextRow();
      ImGui::TableNextColumn();

      ImGui::Text("%lu", sdb.models.size());
      ImGui::TableNextColumn();

      ImGui::Text("%lu", sdb.models.get_write_buffer());

      ImGui::EndTable();
    }
  }

  if (ImGui::CollapsingHeader("Alphas")) {
    if (ImGui::BeginTable("sdb_table_alphas", 2, flags)) {
      ImGui::TableSetupColumn("Size");
      ImGui::TableSetupColumn("Write Buffer");
      ImGui::TableHeadersRow();

      ImGui::TableNextRow();
      ImGui::TableNextColumn();

      ImGui::Text("%lu", sdb.alphas.size());
      ImGui::TableNextColumn();

      ImGui::Text("%lu", sdb.alphas.get_write_buffer());

      ImGui::EndTable();
    }
  }

  ImGui::End();
}