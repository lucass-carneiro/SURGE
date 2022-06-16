#ifndef SURGE_JOB_SYSTEM
#define SURGE_JOB_SYSTEM

#include "log.hpp"

// clang-format off
#include <EASTL/vector.h>
// clang-format on

#include <iostream>
#include <mutex>
#include <thread>

namespace surge {

template <typename allocator_type = EASTLAllocatorType> class job_system {
public:
  job_system(const allocator_type &allocator,
             unsigned int threads = (std::thread::hardware_concurrency() - 1))
      : num_threads(threads), thread_vector(num_threads, allocator), is_running(true) {

    const auto thread_task = [](unsigned int index) {
      global_file_log::instance().log<log_event::message>("Starting worker thread index {}", index);
    };

    for (unsigned int i = 0; auto &thread : thread_vector) {
      thread = std::thread(thread_task, i);
      i++;
    }
  }

  void join_all() {
    for (auto &thread : thread_vector) {
      thread.join();
    }
  }

private:
  const unsigned int num_threads;
  eastl::vector<std::thread, allocator_type> thread_vector;
  bool is_running;

  std::mutex job_mutex;
};

} // namespace surge

#endif