#include "imgui_wrapper.hpp"

#include "window.hpp"

// clang-format off
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
// clang-format on

void surge::imgui::gl::create(create_config &&cfg) noexcept {
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

void surge::imgui::gl::destroy() noexcept {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void surge::imgui::gl::frame_begin() noexcept {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void surge::imgui::gl::frame_end() noexcept {
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

#ifdef SURGE_BUILD_TYPE_Debug

void surge::imgui::gl::texture_database_window(bool *open,
                                               const gl_atom::texture::database &tdb) noexcept {
  // Early out if the window is collapsed, as an optimization.
  if (!ImGui::Begin("Texture Database", open)) {
    ImGui::End();
    return;
  }

  if (begin_table("tdb_table", 4)) {
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

void surge::imgui::gl::sprite_database_window(bool *open,
                                              const gl_atom::sprite::database &sdb) noexcept {
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

#endif SURGE_BUILD_TYPE_Debug

auto surge::imgui::begin(const char *name, bool *open) noexcept -> bool {
  return ImGui::Begin(name, open);
}

void surge::imgui::end() noexcept { ImGui::End(); }

auto surge::imgui::begin_main_menu_bar() noexcept -> bool { return ImGui::BeginMainMenuBar(); }

void surge::imgui::end_main_menu_bar() noexcept { ImGui::EndMainMenuBar(); }

auto surge::imgui::begin_menu(const char *name) noexcept -> bool { return ImGui::BeginMenu(name); }

auto surge::imgui::menu_item(const char *name) noexcept -> bool { return ImGui::MenuItem(name); }

void surge::imgui::end_menu() noexcept { ImGui::EndMenu(); }

auto surge::imgui::begin_table(const char *name, int cols) noexcept -> bool {
  constexpr ImGuiTableFlags flags{ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersV
                                  | ImGuiTableFlags_BordersH
                                  | ImGuiTableFlags_HighlightHoveredColumn};

  return ImGui::BeginTable(name, cols, flags);
}

void surge::imgui::end_table() noexcept { ImGui::EndTable(); }

void surge::imgui::table_setup_column(const char *name) noexcept { ImGui::TableSetupColumn(name); }

void surge::imgui::table_headers_row() noexcept { ImGui::TableHeadersRow(); }

void surge::imgui::table_next_row() noexcept { ImGui::TableNextRow(); }

auto surge::imgui::table_next_column() noexcept -> bool { return ImGui::TableNextColumn(); }

void surge::imgui::text(const char *fmt, ...) noexcept {
  va_list args;
  va_start(args, fmt);
  ImGui::TextV(fmt, args);
  va_end(args);
}

auto surge::imgui::colapsing_header(const char *name) noexcept -> bool {
  return ImGui::CollapsingHeader(name);
}

void surge::imgui::register_mouse_callback(int button, int action, int mods) noexcept {
  ImGui_ImplGlfw_MouseButtonCallback(window::get_window_ptr(), button, action, mods);
}

void surge::imgui::register_mouse_scroll_callback(double xoffset, double yoffset) noexcept {
  ImGui_ImplGlfw_ScrollCallback(window::get_window_ptr(), xoffset, yoffset);
}