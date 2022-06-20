#include "log.hpp"

#include <gtest/gtest.h>

surge::log_manager surge::global_stdout_log_manager{};
surge::log_manager surge::global_file_log_manager{};

auto main(int argc, char **argv) -> int {
  using namespace surge;

  global_stdout_log_manager.startup();
  global_file_log_manager.startup("log.txt");

  ::testing::InitGoogleTest(&argc, argv);
  int test_run_code = RUN_ALL_TESTS();

  global_stdout_log_manager.shutdown();
  global_file_log_manager.shutdown();

  return test_run_code;
}