#ifndef SURGE_TASKS_HPP
#define SURGE_TASKS_HPP

#include <taskflow/taskflow.hpp>

namespace surge::tasks {

auto executor() noexcept -> tf::Executor &;

} // namespace surge::tasks

#endif // SURGE_TASKS_HPP