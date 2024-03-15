#include "tasks.hpp"

static const auto num_workers{std::thread::hardware_concurrency() - 1};

// NOLINTNEXTLINE
static tf::Executor global_task_executor{num_workers};

auto surge::tasks::executor() noexcept -> tf::Executor & { return global_task_executor; }