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

const std::filesystem::path surge::global_file_log_manager::file_path =
    std::filesystem::path{"log.txt"};

const SQInteger surge::global_squirrel_vm::stack_size = 1024;

const std::size_t surge::global_arena_allocator::arena_size = 1024;

inline auto init_all_subsystems() noexcept {
  using namespace surge;
  global_stdout_log_manager::get();
  global_file_log_manager::get();
  global_squirrel_vm::get();
  global_arena_allocator::get();
}

auto main(int argc, char **argv) noexcept -> int {
  using namespace surge;

  init_all_subsystems();

  // Job system
  // TODO: Bug: when the things get destroyed, it tries to print to a file
  // but the log system is already gone and no file is there, only a nullptr
  job_system system(global_arena_allocator::get());
  system.join_all();

  // Command line argument parsing
  const auto cmd_line_args = parse_arguments(argc, argv);
  if (!cmd_line_args) {
    return EXIT_FAILURE;
  }

  // Config script validation and engine context loading
  const auto valid_config_script =
      validate_config_script_path(*(cmd_line_args));
  if (!valid_config_script) {
    return EXIT_FAILURE;
  }

  const auto engine_context_pushed =
      global_squirrel_vm::get().load_context(*(valid_config_script));
  if (!engine_context_pushed) {
    return EXIT_FAILURE;
  }

  // Retrieve configuration values
  const auto window_width =
      global_squirrel_vm::get().surge_retrieve<SQInteger, int>(
          _SC("window_width"));
  if (!window_width.has_value()) {
    log_all<log_event::error>("Invalid window width.");
    return EXIT_FAILURE;
  }

  const auto window_height =
      global_squirrel_vm::get().surge_retrieve<SQInteger, int>(
          _SC("window_height"));
  if (!window_width.has_value()) {
    log_all<log_event::error>("Invalid window height.");
    return EXIT_FAILURE;
  }

  const auto window_name =
      global_squirrel_vm::get().surge_retrieve<const SQChar *>(
          _SC("window_name"));
  if (!window_name.has_value()) {
    return EXIT_FAILURE;
  }

  auto windowed =
      global_squirrel_vm::get().surge_retrieve<SQBool>(_SC("windowed"));
  if (!windowed.has_value()) {
    return EXIT_FAILURE;
  }

  auto window_monitor_index =
      global_squirrel_vm::get().surge_retrieve<SQInteger>(
          _SC("window_monitor_index"));
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
    log_all<log_event::warning>(
        "Unable to set window monitor to {} because there are only {} "
        "monitors. Using default monitor index 0",
        window_monitor_index.value(), monitors.value().second);
    window_monitor_index = std::make_optional(SQInteger{0});
  }

  // GLFW window creation
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  GLFWwindow *window = nullptr;

  // TODO: Isolate this code
  try {
    if (windowed.value() == SQBool{true}) {
      window = glfwCreateWindow(window_width.value(), window_height.value(),
                                window_name.value(), nullptr, nullptr);
    } else if (windowed.value() == SQBool{false}) {
      window = glfwCreateWindow(
          window_width.value(), window_height.value(), window_name.value(),
          (monitors.value().first)[window_monitor_index.value()], nullptr);
    }
  } catch (const std::exception &) {
  }

  if (window == nullptr) {
    glfwTerminate();
    return EXIT_FAILURE;
  }

  // Main loop
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }

  // Normal shutdown
  glfwDestroyWindow(window);
  glfwTerminate();

  return EXIT_SUCCESS;
}