#ifndef SURGE_THREAD_POOL_HPP
#define SURGE_THREAD_POOL_HPP

#include "logging_system/logging_system.hpp"

// clang-format off
#include <taskflow/core/executor.hpp>
#include <taskflow/taskflow.hpp>
// clang-format on

#include <exception>

namespace surge {

class job_system {
public:
  inline static auto get() -> job_system & {
    static job_system js;
    return js;
  }

  [[nodiscard]] inline auto executor() noexcept -> tf::Executor & { return exec; }

  job_system(const job_system &) = delete;
  job_system(job_system &&) = delete;

  auto operator=(job_system) -> job_system & = delete;

  auto operator=(const job_system &) -> job_system & = delete;

  auto operator=(job_system &&) -> job_system & = delete;

  ~job_system() = default;

private:
  job_system() = default;

  tf::Executor exec;
};

} // namespace surge

#endif // SURGE_THREAD_POOL_HPP