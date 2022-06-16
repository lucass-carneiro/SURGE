#include <gtest/gtest.h>

auto main(int argc, char **argv) -> int {

  ::testing::InitGoogleTest(&argc, argv);
  int test_run_code = RUN_ALL_TESTS();

  return test_run_code;
}