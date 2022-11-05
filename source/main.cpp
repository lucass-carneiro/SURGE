#include "cli.hpp"
#include "gui_windows/gui_windows.hpp"
#include "image_loader.hpp"
#include "log.hpp"
#include "lua/lua_vm.hpp"
#include "mesh/sprite.hpp"
#include "opengl/create_program.hpp"
#include "opengl/load_texture.hpp"
#include "safe_ops.hpp"
#include "task_executor.hpp"
#include "thread_allocators.hpp"
#include "window.hpp"

#include <cstddef>
#include <cstdlib>
#include <glm/common.hpp>
#include <vector>

constexpr auto pow2(std::size_t n) noexcept -> std::size_t {
  if (n == 0) {
    return 1;
  } else {
    return 2 * pow2(n - 1);
  }
}

auto main(int argc, char **argv) noexcept -> int {
  using namespace surge;

  // Init log subsystem
  global_log_manager::get().init("./log.txt");
  draw_logo();

  // Command line argument parsing
  const auto cmd_line_args = parse_arguments(argc, argv);
  if (!cmd_line_args) {
    return EXIT_FAILURE;
  }

  // Parameter recovery
  const auto pages{get_arg_long(*cmd_line_args, "--pages")};
  if (!pages) {
    return EXIT_FAILURE;
  }

  const auto max_mem{(*pages) * get_page_size()};

  const auto mem_quota{get_arg_long(*cmd_line_args, "--thread-mem-quota")};
  if (!mem_quota) {
    return EXIT_FAILURE;
  }

  if (*mem_quota < 0 || mem_quota > 100) {
    glog<log_event::error>(" The thread memory quota must be between 0 and 100");
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

  // Memory per thread calculation
  const auto mem_per_thread{(static_cast<float>(*mem_quota) / 100)
                            * (static_cast<float>(max_mem) / static_cast<float>(*num_threads))};
  const auto mem_per_thread_long{static_cast<long>(mem_per_thread)};

  glog<log_event::message>("Memory distribution:\n"
                           "  System pages: {}\n"
                           "  Page size: {} (B)\n"
                           "  Total memory (B): {}\n"
                           "  Number of threads: {}\n"
                           "  Thread memory quota (%): {}\n"
                           "  Memory per thread (B): {}\n"
                           "  Remaining usable memory (B): {}",
                           *pages, get_page_size(), max_mem, *num_threads, *mem_quota,
                           mem_per_thread_long, max_mem - *num_threads * mem_per_thread_long);

  // Main arena initialization
  global_default_allocator::get().init("Global default allocator (mimalloc)");
  global_linear_arena_allocator::get().init(&global_default_allocator::get(), max_mem,
                                            "Global engine arena");

  // Thread allocator initializations (uses global_linear_arena_allocator for the array and for
  // allocators)
  global_thread_allocators::get().init(*num_threads, mem_per_thread_long);

  // Init Lua VM states ( global_linear_arena_allocator holds the array and
  // LuaJIT allocates memory for each state using it's own allocator). TODO: In 64bit architectures,
  // LuaJIT does not allow one to change it's internal allocator. There are workarounds (see
  // XPlane's strategy) using a custom version of the lib but it is complicated. Change this in the
  // future if possible
  global_lua_states::get().init();

  // Init parallel job system
  glog<log_event::message>("Initializing job system with {} workers",
                           *num_threads == 1 ? 1 : *num_threads - 1);
  global_task_executor::get();

  // Init all VMs with the engine configuration
  for (auto i = 0; i < *num_threads; i++) {
    global_task_executor::get().async(do_file_at, i, *config_script_path);
  }
  global_task_executor::get().wait_for_all();

  // Do startup file at the main thread VM
  do_file_at(*num_threads - 1, *startup_script_path);

  // Initialize GLFW
  if (!global_engine_window::get().init()) {
    return EXIT_FAILURE;
  }
  const auto &engine_config{*global_engine_window::get().get_config()};

  // TEMPORARY
  const sprite_position pos{.x = static_cast<float>(engine_config.window_width) / 4,
                            .y = static_cast<float>(engine_config.window_height) / 4,
                            .z = 0.0f,
                            .width = 400.0f,
                            .height = 400.0f};
  const sprite test_sprite(global_thread_allocators::get().back().get(),
                           "/home/lucas/SURGE/resources/images/awesomeface.png", ".png",
                           buffer_usage_hint::static_draw, pos);

  /*******************************
   *     SHADERS/TRANSFORMS      *
   *******************************/
  glog<log_event::message>("Compiling sprite shader");
  const auto sprite_shader{create_program(global_thread_allocators::get().back().get(),
                                          "../shaders/sprite.vert", "../shaders/sprite.frag")};
  if (!sprite_shader) {
    return EXIT_FAILURE;
  }

  // TODO: Let script choose the projection matrix?
  const auto default_2D_orthographic_projection{
      glm::ortho(0.0f, static_cast<float>(engine_config.window_width),
                 static_cast<float>(engine_config.window_height), 0.0f, -1.0f, 1.0f)};

  // The view matrix
  auto default_camera{glm::lookAt(glm::vec3(0.0f, 0.0f, 0.5f), glm::vec3(0.0f, 0.0f, 0.0f),
                                  glm::vec3(0.0f, 1.0f, 0.0f))};

  /*******************************
   *        LOAD CALLBACK        *
   *******************************/
  if (!lua_load_callback(global_lua_states::get().back().get())) {
    return EXIT_FAILURE;
  }

  /*******************************
   *          MAIN LOOP          *
   *******************************/
  while ((global_engine_window::get().frame_timer_reset_and_start(),
          !global_engine_window::get().should_close())) {

    // OpenGL options. TODO: Set by script
    glEnable(GL_DEPTH_TEST);

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Handle events

    // Update states

    // Clear buffers
    global_engine_window::get().clear_framebuffer();
    glClear(GL_DEPTH_BUFFER_BIT);

    // Render user meshes. TODO: Do not pass projectio matrix every time. Set it once.
    test_sprite.draw(*sprite_shader, default_2D_orthographic_projection, default_camera);

    // Render Dear ImGui
    // ImGui::ShowDemoWindow();
    show_main_gui_window();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Present rendering
    global_engine_window::get().swap_buffers();

    // Poll IO events
    global_engine_window::get().poll_events();

    // Compute elapsed time
    global_engine_window::get().frame_timmer_compute_dt();
  }

  return EXIT_SUCCESS;
}