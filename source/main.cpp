#include "allocator.hpp"
#include "cli.hpp"
#include "gui_windows/gui_windows.hpp"
#include "lua/lua_states.hpp"
#include "lua/lua_utils.hpp"
#include "task_executor.hpp"
#include "window.hpp"

#ifdef SURGE_ENABLE_TRACY
#  include <tracy/Tracy.hpp>
#endif

auto main(int argc, char **argv) noexcept -> int {
  using namespace surge;
  using std::printf;

#ifdef SURGE_ENABLE_TRACY
  ZoneScoped;
#endif

  draw_logo();

  // Init allocator subsystem
  init_mimalloc();
  mimalloc_eastl_allocator::get();

  // Init log subsystem
  try {
    log_manager::get();
  } catch (const std::exception &e) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    printf("Unable to initialize logging system: %s", e.what());
    return EXIT_FAILURE;
  }

  // Command line argument parsing
  const auto cmd_line_args = parse_arguments(argc, argv);
  if (!cmd_line_args) {
    return EXIT_FAILURE;
  }

  const auto [config_script_path, startup_script_path] = cmd_line_args.value();

  // Init parallel job system
  global_num_threads::get().init(1);
  global_task_executor::get();

  /* Init Lua VM states
   * LuaJIT allocates memory for each state using it's own allocator). TODO: In 64bit architectures,
   * LuaJIT does not allow one to change it's internal allocator. There are workarounds (see
   * XPlane's strategy) using a custom version of the lib but it is complicated. Change this in the
   * future if possible
   */
  if (!global_lua_states::get().init()) {
    return EXIT_FAILURE;
  }

  // Init all VMs with the engine configuration
  if (!global_lua_states::get().configure(config_script_path)) {
    return EXIT_FAILURE;
  }

  // Do the startup file in VM 0 (main thread)
  if (!do_file_at_idx(0, startup_script_path)) {
    return EXIT_FAILURE;
  }

  // Initialize GLFW
  if (!global_engine_window::get().init()) {
    return EXIT_FAILURE;
  }

  /*******************************
   *      PRE LOOP CALLBACK      *
   *******************************/
  if (!lua_pre_loop_callback(global_lua_states::get().at(0).get())) {
    return EXIT_FAILURE;
  }

  /*******************************
   *          MAIN LOOP          *
   *******************************/
  while ((global_engine_window::get().frame_timer_reset_and_start(),
          !global_engine_window::get().should_close())) {

    // Poll IO events
    global_engine_window::get().poll_events();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();

    // This is required to fix a failed assartion in ImGui. Stupid, but works
    ImGui::GetIO().DeltaTime = 1.0e-5;

    ImGui::NewFrame();

    /*
     * Hot reload startup script. TODO: Maybe this should be better, like user controlled?
     */
    if (global_engine_window::get().get_key(GLFW_KEY_F5) == GLFW_PRESS) {
      if (!do_file_at_idx(0, startup_script_path)) {
        return EXIT_FAILURE;
      }
    }

    /*
     * Lua update callback
     */
    lua_update_callback(global_lua_states::get().at(0).get());

    // Clear buffers
    global_engine_window::get().clear_framebuffer();
    glClear(GL_DEPTH_BUFFER_BIT);

    /*
     * Lua draw callback
     */
    lua_draw_callback(global_lua_states::get().at(0).get());

    // Render Dear ImGui
    // ImGui::ShowDemoWindow();
    // show_main_gui_window();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Present rendering
    global_engine_window::get().swap_buffers();

#ifdef SURGE_ENABLE_TRACY
    FrameMark;
#endif

    // Compute elapsed time
    global_engine_window::get().frame_timmer_compute_dt();
  }

  return EXIT_SUCCESS;
}