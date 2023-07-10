#ifndef SURGE_JOB_SYSTEM
#define SURGE_JOB_SYSTEM

// clang-format off
#include <taskflow/core/executor.hpp>
#include <taskflow/taskflow.hpp>
// clang-format on

namespace surge::job_system {

extern tf::Executor executor;

} // namespace surge::job_system

#endif // SURGE_JOB_SYSTEM