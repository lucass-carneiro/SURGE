#ifndef SURGE_CORE_TASKS_HPP
#define SURGE_CORE_TASKS_HPP

#include <taskflow/taskflow.hpp>

namespace surge::tasks {

class executor {
private:
  executor() = default;
  ~executor() = default;

public:
  static auto get() -> tf::Executor &;

  executor(const executor &) = delete;
  executor &operator=(const executor &) = delete;
};

} // namespace surge::tasks

#endif // SURGE_CORE_TASKS_HPP