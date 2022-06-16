#include "arena_allocator.hpp"
#include "cli.hpp"
#include "job_system.hpp"
#include "log.hpp"
#include "safe_ops.hpp"
#include "squirrel_bindings.hpp"
#include "window.hpp"

// clang-format off
#include <GLFW/glfw3.h>
#include <EASTL/vector.h>
// clang-format on

#include <cstdlib>
#include <exception>

auto main(int argc, char **argv) -> int {
  using namespace surge;

  try {
    // Command line argument parsing and
    const auto cmd_line_args = parse_arguments(argc, argv);
    if (!cmd_line_args) {
      return EXIT_FAILURE;
    }

    // Config script validation and engine context loading
    const auto valid_config_script = validate_config_script_path(*(cmd_line_args));
    if (!valid_config_script) {
      return EXIT_FAILURE;
    }

    const auto engine_context_pushed
        = push_engine_context(global_squirrel_vm::instance().get(), *(valid_config_script));
    if (!engine_context_pushed) {
      return EXIT_FAILURE;
    }

    // Retrieve configuration values
    const auto window_width = retrieve_window_config<SQInteger>(
        global_squirrel_vm::instance().get(), _SC("window_width"));
    if (!window_width.has_value()) {
      return EXIT_FAILURE;
    }

    const auto int_widonw_width = safe_cast<int>(window_width.value());
    if (!int_widonw_width) {
      log_all<log_event::error>("Invalid window width.");
      return EXIT_FAILURE;
    }

    const auto window_height = retrieve_window_config<SQInteger>(
        global_squirrel_vm::instance().get(), _SC("window_height"));
    if (!window_height.has_value()) {
      return EXIT_FAILURE;
    }

    const auto int_widonw_height = safe_cast<int>(window_height.value());
    if (!int_widonw_height) {
      log_all<log_event::error>("Invalid window height.");
      return EXIT_FAILURE;
    }

    auto window_name = retrieve_window_config<const SQChar *>(global_squirrel_vm::instance().get(),
                                                              _SC("window_name"));
    if (!window_name.has_value()) {
      return EXIT_FAILURE;
    }

    auto windowed
        = retrieve_window_config<SQBool>(global_squirrel_vm::instance().get(), _SC("windowed"));
    if (!windowed.has_value()) {
      return EXIT_FAILURE;
    }

    auto window_monitor_index = retrieve_window_config<SQInteger>(
        global_squirrel_vm::instance().get(), _SC("window_monitor_index"));
    if (!window_monitor_index.has_value()) {
      return EXIT_FAILURE;
    }

    // GLFW callbacks
    glfwSetErrorCallback(surge::glfw_error_callback);

    // GLFW initialization
    if (glfwInit() != GLFW_TRUE) {
      return EXIT_FAILURE;
    }

    auto monitors = querry_available_monitors();
    if (!monitors.has_value()) {
      glfwTerminate();
      return EXIT_FAILURE;
    }
    if (window_monitor_index >= monitors.value().second) {
      log_all<log_event::warning>("Unable to set window monitor to {} because there are only {} "
                                  "monitors. Using default monitor index 0",
                                  window_monitor_index.value(), monitors.value().second);
      window_monitor_index = std::make_optional(SQInteger{0});
    }

    // GLFW window creation
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow *window = nullptr;

    if (windowed.value() == SQBool{true}) {
      window = glfwCreateWindow(int_widonw_width.value(), int_widonw_height.value(),
                                window_name.value(), nullptr, nullptr);
    } else if (windowed.value() == SQBool{false}) {
      window = glfwCreateWindow(int_widonw_width.value(), int_widonw_height.value(),
                                window_name.value(),
                                (monitors.value().first)[window_monitor_index.value()], nullptr);
    }

    // Memory arena
    arena_allocator arena("main() arena allocator", 1024);

    // Job system
    job_system system(arena);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    system.join_all();

    return EXIT_SUCCESS;

  } catch (const std::exception &e) {
    std::cout << "Unknow error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}