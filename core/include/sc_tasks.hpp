#ifndef SURGE_CORE_TASKS_HPP
#define SURGE_CORE_TASKS_HPP

#include <taskflow/taskflow.hpp>

namespace surge::tasks {

auto executor() -> tf::Executor &;

} // namespace surge::tasks

#endif // SURGE_CORE_TASKS_HPP