#ifndef SURGE_THREAD_POOL_HPP
#define SURGE_THREAD_POOL_HPP

#include "logging_system/logging_system.hpp"

// clang-format off
#include <taskflow/core/executor.hpp>
#include <taskflow/taskflow.hpp>
// clang-format on

#include <exception>

namespace surge {

class global_num_threads {
public:
  inline static auto get() noexcept -> global_num_threads & {
    static global_num_threads nt;
    return nt;
  }

  inline void init(long nt) noexcept { num_threads = nt; }
  [[nodiscard]] inline auto count() const noexcept -> long { return num_threads; }

  global_num_threads(const global_num_threads &) = delete;
  global_num_threads(global_num_threads &&) = delete;

  auto operator=(global_num_threads) -> global_num_threads & = delete;

  auto operator=(const global_num_threads &) -> global_num_threads & = delete;

  auto operator=(global_num_threads &&) -> global_num_threads & = delete;

  ~global_num_threads() = default;

private:
  global_num_threads() = default;
  long num_threads{0};
};

class global_task_executor {
public:
  inline static auto get() noexcept -> tf::Executor & {
    try {
      static tf::Executor executor(
          global_num_threads::get().count() > 1 ? global_num_threads::get().count() : 1);
      return executor;
    } catch (const std::exception &e) {
      log_error("Global taskflow executor returned an exception: {}", e.what());
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