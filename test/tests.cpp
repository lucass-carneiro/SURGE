#include "image_loader.hpp"
#include "log.hpp"
#include "squirrel_bindings.hpp"
#include "window.hpp"

#include <gtest/gtest.h>

const std::filesystem::path surge::global_file_log_manager::file_path
    = std::filesystem::path{"log.txt"};

const SQInteger surge::global_squirrel_vm::stack_size = 1024;

const std::size_t surge::global_linear_arena_allocator::capacity = 2048;

const std::size_t surge::global_engine_window::subsystem_allocator_capacity = 128;

const std::size_t surge::global_image_loader::subsystem_allocator_capacity = 512;
const std::size_t surge::global_image_loader::persistent_allocator_capacity = 128;
const std::size_t surge::global_image_loader::volatile_allocator_capacity = 128;

constexpr auto pow2(std::size_t n) noexcept -> std::size_t {
  if (n == 0) {
    return 1;
  } else {
    return 2 * pow2(n - 1);
  }
}

const std::size_t surge::global_stack_allocator::capacity = pow2(12);

inline auto init_all_subsystems() noexcept {
  using namespace surge;
  global_stdout_log_manager::get();
  global_file_log_manager::get();
  global_squirrel_vm::get();
}

auto main(int argc, char **argv) -> int {
  using namespace surge;

  init_all_subsystems();

  global_stdout_log_manager::get();
  global_file_log_manager::get();

  ::testing::InitGoogleTest(&argc, argv);
  int test_run_code = RUN_ALL_TESTS();

  return test_run_code;
}