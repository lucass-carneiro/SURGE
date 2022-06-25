#include "log.hpp"

#include <gtest/gtest.h>

const std::filesystem::path surge::global_file_log_manager::file_path =
    std::filesystem::path{"test_log.txt"};

auto main(int argc, char **argv) -> int {
  using namespace surge;

  global_stdout_log_manager::get();
  global_file_log_manager::get();

  ::testing::InitGoogleTest(&argc, argv);
  int test_run_code = RUN_ALL_TESTS();

  return test_run_code;
}