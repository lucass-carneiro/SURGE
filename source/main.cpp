#include "allocator.hpp"
#include "cli.hpp"
#include "gui_windows/gui_windows.hpp"
#include "lua/lua_states.hpp"
#include "lua/lua_utils.hpp"
#include "task_executor.hpp"
#include "timer_system/timer_system.hpp"
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

  // Logo
  draw_logo();

  // Init allocator subsystem
  init_mimalloc();

  // Init log subsystem
  if (!logger::init()) {
    return EXIT_FAILURE;
  }

  // Command line argument parsing
  const auto cmd_line_args = parse_arguments(argc, argv);
  if (!cmd_line_args) {
    return EXIT_FAILURE;
  }

  const auto [config_script_path, startup_script_path] = cmd_line_args.value();

  // Init parallel job system
  try {
    job_system::get();
  } catch (const std::exception &e) {
    log_error("Unable to initialize parallel job system {}", e.what());
    return EXIT_FAILURE;
  }

  /* Init Lua VM states
   * LuaJIT allocates memory for each state using it's own allocator). TODO: In 64bit architectures,
   * LuaJIT does not allow one to change it's internal allocator. There are workarounds (see
   * XPlane's strategy) using a custom version of the lib but it is complicated. Change this in the
   * future if possible
   */
  try {
    global_lua_states::get();
  } catch (const std::exception &e) {
    log_error("Unable to initialize lua states {}", e.what());
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

  auto dt_start{std::chrono::steady_clock::now()};

  while ((frame_timer::begin(), !global_engine_window::get().should_close())) {

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
    lua_update_callback(
        global_lua_states::get().at(0).get(),
        std::chrono::duration<double>{std::chrono::steady_clock::now() - dt_start}.count());
    dt_start = std::chrono::steady_clock::now();

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

    // Compute elapsed frame time
    frame_timer::end();
    log_info("frame time {}", frame_timer::duration());

#ifdef SURGE_ENABLE_TRACY
    FrameMark;
#endif
  }

  return EXIT_SUCCESS;
}