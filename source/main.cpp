#include "allocator.hpp"
#include "cli.hpp"
#include "gui_windows/gui_windows.hpp"
#include "log.hpp"
#include "lua/lua_states.hpp"
#include "lua/lua_utils.hpp"
#include "task_executor.hpp"
#include "window.hpp"

auto main(int argc, char **argv) noexcept -> int {
  using namespace surge;

  // Init allocator subsystem
  init_mimalloc();
  eastl_allocator::get();

  // Init log subsystem
  global_log_manager::get().init("log.txt");
  draw_logo();

  // Command line argument parsing
  const auto cmd_line_args = parse_arguments(argc, argv);
  if (!cmd_line_args) {
    return EXIT_FAILURE;
  }

  const auto num_threads{get_arg_long(*cmd_line_args, "--num-threads")};
  if (!num_threads) {
    return EXIT_FAILURE;
  }

  const auto hardware_concurrency{std::thread::hardware_concurrency()};
  if (*num_threads < 0 || *num_threads > hardware_concurrency) {
    glog<log_event::error>("The number of threads must be in the range [{},{}]", 0,
                           hardware_concurrency);
    return EXIT_FAILURE;
  }

  const auto config_script_path = get_file_path(*cmd_line_args, "<config-script>", ".lua");
  if (!config_script_path) {
    return EXIT_FAILURE;
  }

  const auto startup_script_path = get_file_path(*cmd_line_args, "<startup-script>", ".lua");
  if (!startup_script_path) {
    return EXIT_FAILURE;
  }

  // Init parallel job system
  const auto num_workers{*num_threads > 1 ? *num_threads - 1 : *num_threads};
  glog<log_event::message>("Initializing job system with {} workers", num_workers);
  global_num_threads::get().init(num_workers);
  global_task_executor::get();

  /* Init Lua VM states
   * LuaJIT allocates memory for each state using it's own allocator). TODO: In 64bit architectures,
   * LuaJIT does not allow one to change it's internal allocator. There are workarounds (see
   * XPlane's strategy) using a custom version of the lib but it is complicated. Change this in the
   * future if possible
   */
  global_lua_states::get().init();

  // Init all VMs with the engine configuration
  if (!global_lua_states::get().configure(*config_script_path)) {
    return EXIT_FAILURE;
  }

  // Do the startup file in VM 0 (main thread)
  if (!do_file_at(0, *startup_script_path)) {
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
      if (!do_file_at(0, *startup_script_path)) {
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
    show_main_gui_window();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Present rendering
    global_engine_window::get().swap_buffers();

    // Compute elapsed time
    global_engine_window::get().frame_timmer_compute_dt();

    // Poll IO events
    global_engine_window::get().poll_events();
  }

  return EXIT_SUCCESS;
}