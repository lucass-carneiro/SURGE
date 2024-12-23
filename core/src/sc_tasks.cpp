#include "sc_tasks.hpp"

auto surge::tasks::executor::get() -> tf::Executor & {
  static tf::Executor e{std::thread::hardware_concurrency() - 1};
  return e;
}