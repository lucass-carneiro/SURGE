#include "thread_allocators.hpp"

#include "allocators/stack_allocator.hpp"
#include "log.hpp"
#include "safe_ops.hpp"
#include "static_map.hpp"

#include <exception>

// This allows one to use different allocator names for each thread index without allocating new
// memory for string manipulation. TODO: It's kind of a hack. Maybe there is a better solution?
using allocator_name_t = surge::static_map<unsigned int, const char *, 20>;

// clang-format off
constexpr const allocator_name_t allocator_name_map{{{
      {0,  "Thread 0 stack allocator"},
      {1,  "Thread 1 stack allocator"},
      {2,  "Thread 2 stack allocator"},
      {3,  "Thread 3 stack allocator"},
      {4,  "Thread 4 stack allocator"},
      {5,  "Thread 5 stack allocator"},
      {6,  "Thread 6 stack allocator"},
      {7,  "Thread 7 stack allocator"},
      {8,  "Thread 8 stack allocator"},
      {9,  "Thread 9 stack allocator"},
      {10, "Thread 10 stack allocator"},
      {11, "Thread 11 stack allocator"},
      {12, "Thread 12 stack allocator"},
      {13, "Thread 13 stack allocator"},
      {14, "Thread 14 stack allocator"},
      {15, "Thread 15 stack allocator"},
      {16, "Thread 16 stack allocator"},
      {17, "Thread 17 stack allocator"},
      {18, "Thread 18 stack allocator"},
      {19, "Thread 19 stack allocator"}
      }}};
// clang-format on

void surge::global_thread_allocators::init(unsigned int nt, long mpt) noexcept {
  glog<log_event::message>("Initializing thread allocators");

  num_threads = nt;
  memory_per_thread = mpt;

  // Step 1: Allocate memory for the array of allocator pointers
  allocator_array.reserve(num_threads);

  // Setep 2: Allocate memory for each allocator and store the pointer
  for (unsigned int i = 0; i < num_threads; i++) {
    void *buffer = parent_allocator->malloc(sizeof(stack_allocator));
    allocator_array.push_back(stack_allocator_ptr{new (buffer) stack_allocator,
                                                  [&](void *ptr) { parent_allocator->free(ptr); }});

    // Setep 3: Initialize each stack_allocator
    allocator_array[i]->init(parent_allocator, memory_per_thread, allocator_name_map[i]);
  }
}

surge::global_thread_allocators::~global_thread_allocators() noexcept {
  auto int_num_threads{safe_cast<int>(num_threads)};

  // We need to explicitly destroy each allocator in the array since the destructor of the
  // unique_ptr frees the memory used to create the pointer it's holding but not the memory that the
  // object pointed to by the pointer is holding
  for (int i = (*int_num_threads - 1); i >= 0; i--) {
    allocator_array[i]->~stack_allocator();
  }
}

auto surge::global_thread_allocators::at(std::size_t i) noexcept -> stack_allocator_ptr & {
  try {
    return allocator_array.at(i);
  } catch (const std::exception &e) {
    glog<log_event::error>(
        "Uanble to acess global thread allocator at index {}: {}. Returning main thread allocator.",
        i, e.what());
  }
  return allocator_array.back();
}

auto surge::global_thread_allocators::back() noexcept -> stack_allocator_ptr & {
  return allocator_array.back();
}