#include "allocators/linear_arena_allocator.hpp"

#include <cstddef>

const std::size_t default_alignment = alignof(std::max_align_t);

#ifdef SURGE_DEBUG_MEMORY
surge::linear_arena_allocator::linear_arena_allocator(base_allocator &pa, std::size_t capacity,
                                                      const char *debug_name)
    : parent_allocator{pa},
      requested_arena_capacity{capacity},
      actual_arena_capacity{align_alloc_size(requested_arena_capacity, default_alignment)},
      allocator_debug_name{debug_name},
      arena_buffer{static_cast<std::byte *>(
                       parent_allocator.aligned_alloc(default_alignment, actual_arena_capacity)),
                   [this](void *p) -> void { this->parent_allocator.free(p); }} {}
#else
surge::linear_arena_allocator::linear_arena_allocator(base_allocator &pa, std::size_t capacity)
    : parent_allocator{pa},
      requested_arena_capacity{capacity},
      actual_arena_capacity{align_alloc_size(requested_arena_capacity, default_alignment)},
      arena_buffer{
          static_cast<std::byte *>(pa.aligned_alloc(default_alignment, actual_arena_capacity))} {}
#endif

surge::linear_arena_allocator::~linear_arena_allocator() {
  arena_buffer.reset();

#ifdef SURGE_DEBUG_MEMORY
  log_all<log_event::memory>("Allocator \"{}\"  was destroyed with {} remaining allocations.",
                             allocator_debug_name, allocation_counter);
#endif
}

[[nodiscard]] auto surge::linear_arena_allocator::aligned_alloc(std::size_t alignment,
                                                                std::size_t size) noexcept
    -> void * {

#ifdef SURGE_DEBUG_MEMORY
  // Precondition 1: alignment must be a power of 2
  if (!is_pow_2(alignment)) {
    log_all<log_event::error>("In allocator \"{}\"(aligned_alloc): Allocation of alignment {} "
                              "requested, which is not a power of 2",
                              allocator_debug_name, alignment);

    return nullptr;
  }

  // Precondition 2: alignment is a multiple of sizeof(void*)
  if ((alignment % sizeof(void *)) != 0) {
    log_all<log_event::error>("In allocator \"{}\"(aligned_alloc): Allocation of alignment {} "
                              "requested, which is not a multiple of sizeof(void*) ({})",
                              allocator_debug_name, alignment, sizeof(void *));

    return nullptr;
  }

  // Precondition 3: size is a non zero integral multiple of alignment
  if (size == 0 || (size % alignment != 0)) {
    log_all<log_event::error>("In allocator \"{}\"(aligned_alloc): Allocation of size {} "
                              "requested, "
                              "which is not a non zero integral muliple of the alignment {}",
                              allocator_debug_name, size, alignment);

    return nullptr;
  }
#endif

  const auto current_index = free_index;
  const auto bumped_index = current_index + size;

  if (bumped_index >= actual_arena_capacity) {
#ifdef SURGE_DEBUG_MEMORY
    log_all<log_event::error>(
        "In allocator \"{}\"(aligned_alloc): Allocation of size {} requested "
        "exceeds the arena capacity of (requested, actual, remaining) ({}, {}, "
        "{}) Bytes",
        allocator_debug_name, size, requested_arena_capacity, actual_arena_capacity,
        actual_arena_capacity - free_index);
#endif
    return nullptr;
  }

  // Allocation succesfull
  free_index = bumped_index;
  std::byte *start_ptr = &(arena_buffer.get()[current_index]);

#ifdef SURGE_DEBUG_MEMORY
  log_all<log_event::memory>("Allocator \"{}\" allocation summary:\n"
                             "  Allocated size {}\n"
                             "  Internal range ({},{})\n"
                             "  RAM address {:#x}",
                             allocator_debug_name, size, current_index, free_index - 1,
                             // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                             reinterpret_cast<std::uintptr_t>(start_ptr));

  allocation_counter++;
#endif

  return static_cast<void *>(start_ptr);
}

[[nodiscard]] auto surge::linear_arena_allocator::malloc(std::size_t size) noexcept -> void * {

  // Precondition 1: size must be non null
  if (size == 0) {
#ifdef SURGE_DEBUG_MEMORY
    log_all<log_event::error>("In allocator \"{}\"(malloc): Allocation of size 0 is ill defined",
                              allocator_debug_name);
#endif
    return nullptr;
  }

  const auto actual_alloc_size = align_alloc_size(size, default_alignment);

  return this->aligned_alloc(default_alignment, actual_alloc_size);
}

[[nodiscard]] auto surge::linear_arena_allocator::calloc(std::size_t num, std::size_t size) noexcept
    -> void * {

  // Precondition 1: num must be non null
  if (num == 0) {
#ifdef SURGE_DEBUG_MEMORY
    log_all<log_event::error>("In allocator \"{}\"(calloc): Allocation of 0 elements ill defined",
                              allocator_debug_name);
#endif

    return nullptr;
  }

  // Precondition 2: size must be non null
  if (size == 0) {
#ifdef SURGE_DEBUG_MEMORY
    log_all<log_event::error>("In allocator \"{}\"(calloc): Allocation of size "
                              "0 elements ill defined",
                              allocator_debug_name);
#endif

    return nullptr;
  }

  const auto intended_alloc_size{num * size};
  const auto actual_alloc_size = align_alloc_size(intended_alloc_size, default_alignment);

  auto memory = this->aligned_alloc(default_alignment, actual_alloc_size);

  if (memory != nullptr) {
    std::memset(memory, 0, actual_alloc_size);
  }

  return memory;
}

[[nodiscard]] auto surge::linear_arena_allocator::realloc(void *ptr, std::size_t new_size) noexcept
    -> void * {
  this->free(ptr);
  return malloc(new_size);
}

void surge::linear_arena_allocator::free(void *ptr) noexcept {
#ifdef SURGE_DEBUG_MEMORY
  log_all<log_event::memory>("Allocator \"{}\" released address {:#x}", allocator_debug_name,
                             // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                             reinterpret_cast<std::uintptr_t>(ptr));

  allocation_counter--;
#endif
}

void surge::linear_arena_allocator::save() noexcept {
  saved_state.saved = true;
  saved_state.allocation_counter = allocation_counter;
  saved_state.free_index = free_index;

#ifdef SURGE_DEBUG_MEMORY
  log_all<log_event::memory>("Allocator \"{}\": Saving state:\n"
                             "  free index: {}\n"
                             "  allocation counter: {}",
                             allocator_debug_name, free_index, allocation_counter);
#endif
}

void surge::linear_arena_allocator::restore() noexcept {
  if (saved_state.saved) {
    allocation_counter = saved_state.allocation_counter;
    free_index = saved_state.free_index;
    saved_state.saved = false;

#ifdef SURGE_DEBUG_MEMORY
    log_all<log_event::memory>("Allocator \"{}\": Restoring state:\n"
                               "  free index: {}\n"
                               "  allocation counter: {}",
                               allocator_debug_name, free_index, allocation_counter);
#endif
  }
}