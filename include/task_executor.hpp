#ifndef SURGE_THREAD_POOL_HPP
#define SURGE_THREAD_POOL_HPP

#include "allocators/linear_arena_allocator.hpp"
#include "log.hpp"
#include "thread_allocators.hpp"

// clang-format off
#include <taskflow/core/executor.hpp>
#include <taskflow/taskflow.hpp>
// clang-format on

#include <exception>

namespace surge {

class global_task_executor {
public:
  static auto get() noexcept -> tf::Executor & {
    try {
      static tf::Executor executor(global_thread_allocators::get().get_num_threads() - 1);
      return executor;
    } catch (const std::exception &e) {
      glog<log_event::error>("Global taskflow executor returnd an exception: {}", e.what());
      std::terminate();
    }
  }

  global_task_executor(const global_task_executor &) = delete;
  global_task_executor(global_task_executor &&) = delete;

  auto operator=(global_task_executor) -> global_task_executor & = delete;

  auto operator=(const global_task_executor &) -> global_task_executor & = delete;

  auto operator=(global_task_executor &&) -> global_task_executor & = delete;

  ~global_task_executor() = default;

private:
  global_task_executor() = default;
};

} // namespace surge

#endif // SURGE_THREAD_POOL_HPP