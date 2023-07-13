#include "allocator.hpp"
#include "cli.hpp"
#include "lua/lua_states.hpp"
#include "lua/lua_utils.hpp"
#include "timer_system/timer_system.hpp"
#include "window.hpp"

#include <taskflow/core/executor.hpp>
#include <taskflow/taskflow.hpp>

#ifdef SURGE_ENABLE_TRACY
#  include <tracy/Tracy.hpp>
#endif

auto main(int argc, char **argv) noexcept -> int {
  using namespace surge;

#ifdef SURGE_ENABLE_TRACY
  ZoneScoped;
#endif

  // Init allocator
  init_mimalloc();

  // Logo
  draw_logo();

  // Command line argument parsing
  const auto cmd_line_args = parse_arguments(argc, argv);
  if (!cmd_line_args) {
    return EXIT_FAILURE;
  }

  const auto [config_script_path, startup_script_path] = cmd_line_args.value();

  // Init Job system
  tf::Executor task_executor{};

  /* Init Lua VM states
   * LuaJIT allocates memory for each state using it's own allocator). TODO: In 64bit architectures,
   * LuaJIT does not allow one to change it's internal allocator. There are workarounds (see
   * XPlane's strategy) using a custom version of the lib but it is complicated. Change this in the
   * future if possible
   */
  if (!lua_states::init(task_executor)) {
    return EXIT_FAILURE;
  }

  // Init all VMs with the engine configuration
  if (!lua_states::configure(config_script_path)) {
    return EXIT_FAILURE;
  }

  // Do the startup file in VM 0 (main thread)
  if (!do_file_at_idx(0, startup_script_path)) {
    return EXIT_FAILURE;
  }

  // Retrieve engine config
  const auto engine_config = lua_get_engine_config(lua_states::at(0).get());
  if (!engine_config) {
    return EXIT_FAILURE;
  }

  // Initialize GLFW and game window
  if (!engine_window::init(*engine_config)) {
    return EXIT_FAILURE;
  }

  /*******************************
   *      PRE LOOP CALLBACK      *
   *******************************/
  if (!lua_pre_loop_callback(lua_states::at(0).get())) {
    return EXIT_FAILURE;
  }

  /*******************************
   *          MAIN LOOP          *
   *******************************/

  frame_timer::frame_timer_data frame_clock{std::chrono::steady_clock::now(), 0};
  auto dt_start{std::chrono::steady_clock::now()};

  while ((frame_timer::begin(frame_clock), !glfwWindowShouldClose(engine_window::window.get()))) {
    glfwPollEvents();

    /*
     * Hot reload startup script. TODO: Maybe this should be better, like user controlled?
     */
    if (glfwGetKey(engine_window::window.get(), GLFW_KEY_F5) == GLFW_PRESS) {
      if (!do_file_at_idx(0, startup_script_path)) {
        return EXIT_FAILURE;
      }
    }

    /*
     * Lua update callback
     */
    lua_update_callback(
        lua_states::at(0).get(),
        std::chrono::duration<double>{std::chrono::steady_clock::now() - dt_start}.count());
    dt_start = std::chrono::steady_clock::now();

    // Clear buffers
    glClearColor(gsl::narrow_cast<GLfloat>(engine_config->clear_color[0]),
                 gsl::narrow_cast<GLfloat>(engine_config->clear_color[1]),
                 gsl::narrow_cast<GLfloat>(engine_config->clear_color[2]),
                 gsl::narrow_cast<GLfloat>(engine_config->clear_color[3]));
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);

    /*
     * Lua draw callback
     */
    lua_draw_callback(lua_states::at(0).get());

    // Compute elapsed frame time
    frame_timer::end(frame_clock);

    glfwSwapBuffers(engine_window::window.get());

#ifdef SURGE_ENABLE_TRACY
    FrameMark;
#endif
  }

  log_info("Deleting window.");
  engine_window::window.reset();
  glfwTerminate();

  return EXIT_SUCCESS;
}