#include "allocators/stack_allocator.hpp"

#include "allocators/allocators.hpp"
#include "log.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>

const std::size_t default_alignment = alignof(std::max_align_t);

#ifdef SURGE_DEBUG_MEMORY
surge::stack_allocator::stack_allocator(base_allocator &pa, std::size_t capacity,
                                        const char *debug_name)
    : parent_allocator{pa},
      requested_arena_capacity{capacity},
      actual_arena_capacity{align_alloc_size(requested_arena_capacity, default_alignment)},
      allocator_debug_name{debug_name},
      arena_buffer{static_cast<std::byte *>(
                       parent_allocator.aligned_alloc(default_alignment, actual_arena_capacity)),
                   [this](void *p) -> void { this->parent_allocator.free(p); }} {}
#else
surge::stack_allocator::stack_allocator(base_allocator &pa, std::size_t capacity)
    : parent_allocator{pa},
      requested_arena_capacity{capacity},
      actual_arena_capacity{align_alloc_size(requested_arena_capacity, default_alignment)},
      arena_buffer{
          static_cast<std::byte *>(pa.aligned_alloc(default_alignment, actual_arena_capacity))} {}
#endif

surge::stack_allocator::~stack_allocator() {
  arena_buffer.reset();

#ifdef SURGE_DEBUG_MEMORY
  log_all<log_event::memory>("Allocator \"{}\"  was destroyed with {} remaining allocations.",
                             allocator_debug_name, allocation_counter);
#endif
}

auto surge::stack_allocator::is_valid(void *ptr) noexcept -> bool {
  // 1. It is not null
  if (ptr == nullptr) {
    return false;
  }

  // 2. Is inside the internal buffer
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto start_address{reinterpret_cast<std::uintptr_t>(arena_buffer.get())};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto free_address{reinterpret_cast<std::uintptr_t>(&(arena_buffer.get()[free_index]))};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto block_address{reinterpret_cast<std::uintptr_t>(ptr)};

  if (!(start_address + header_size <= block_address && block_address < free_address)) {
    return false;
  }

  // 3. Has a header <= the allocation counter.
  if (!(read_header(ptr) <= allocation_counter)) {
    return false;
  }

  return true;
}

auto surge::stack_allocator::ptr_to_idx(void *ptr) const noexcept
    -> std::tuple<std::size_t, std::size_t> {

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto buffer_start{reinterpret_cast<std::uintptr_t>(arena_buffer.get())};

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto data_start{reinterpret_cast<std::uintptr_t>(ptr)};

  const std::size_t data_idx{data_start - buffer_start};
  const std::size_t header_idx{data_idx - header_size};

  return std::make_tuple(header_idx, data_idx);
}

auto surge::stack_allocator::read_header(void *ptr) const noexcept -> std::size_t {
  const auto idxs{ptr_to_idx(ptr)};

  std::size_t header{0};
  std::memcpy(&header, &(arena_buffer.get()[std::get<0>(idxs)]), header_size);

  return header;
}

auto surge::stack_allocator::is_last_block(void *ptr) noexcept -> bool {
  const auto header{read_header(ptr)};
  return allocation_counter == header;
}

[[nodiscard]] auto surge::stack_allocator::aligned_alloc(std::size_t alignment,
                                                         std::size_t size) noexcept -> void * {

  // Precondition 1: alignment must be a power of 2
  if (!is_pow_2(alignment)) {
#ifdef SURGE_DEBUG_MEMORY
    log_all<log_event::error>("In allocator \"{}\"(aligned_alloc): Allocation of alignment {} "
                              "requested, which is not a power of 2",
                              allocator_debug_name, alignment);
#endif

    return nullptr;
  }

  // Precondition 2: alignment is a multiple of sizeof(void*)
  if ((alignment % sizeof(void *)) != 0) {
#ifdef SURGE_DEBUG_MEMORY
    log_all<log_event::error>("In allocator \"{}\"(aligned_alloc): Allocation of alignment {} "
                              "requested, which is not a multiple of sizeof(void*) ({})",
                              allocator_debug_name, alignment, sizeof(void *));
#endif

    return nullptr;
  }

  // Precondition 3: size is a non zero integral multiple of alignment
  if (size == 0 || (size % alignment != 0)) {
#ifdef SURGE_DEBUG_MEMORY
    log_all<log_event::error>("In allocator \"{}\"(aligned_alloc): Allocation of size {} "
                              "requested, "
                              "which is not a non zero integral muliple of the alignment {}",
                              allocator_debug_name, size, alignment);
#endif

    return nullptr;
  }

  const auto header_start_idx = free_index;
  const auto data_start_idx = header_start_idx + header_size;
  const auto data_end_idx = data_start_idx + size;

  if (data_end_idx >= actual_arena_capacity) {
#ifdef SURGE_DEBUG_MEMORY
    log_all<log_event::error>(
        "In allocator \"{}\"(aligned_alloc): Allocation of size {} (+ {} header) requested "
        "exceeds the stack capacity of (requested, actual, remaining) ({}, {}, "
        "{}) Bytes",
        allocator_debug_name, size, header_size, requested_arena_capacity, actual_arena_capacity,
        actual_arena_capacity - free_index);
#endif
    return nullptr;
  }

  // Allocation succesfull
  allocation_counter++;
  free_index = data_end_idx;
  std::memcpy(&(arena_buffer.get()[header_start_idx]), &allocation_counter, header_size);
  std::byte *start_ptr = &(arena_buffer.get()[data_start_idx]);

#ifdef SURGE_DEBUG_MEMORY
  log_all<log_event::memory>("Allocator \"{}\" allocation summary:\n"
                             "  Allocated size {}\n"
                             "  Internal range ({},{})\n"
                             "  Alloction count {}\n"
                             "  RAM address {:#x}",
                             allocator_debug_name, size, data_start_idx, data_end_idx - 1,
                             allocation_counter,
                             // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                             reinterpret_cast<std::uintptr_t>(start_ptr));
#endif

  return start_ptr;
}

void surge::stack_allocator::free(void *ptr) noexcept {
#ifdef SURGE_DEBUG_MEMORY
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto uint_ptr = reinterpret_cast<std::uintptr_t>(ptr);
#endif

  if (!is_valid(ptr)) {
#ifdef SURGE_DEBUG_MEMORY
    log_all<log_event::warning>("In allocator \"{}\"(free): Block pointed by address {:#x} is "
                                "invalid. Ignoring free request",
                                allocator_debug_name, uint_ptr);
#endif
    return;
  }

  if (!is_last_block(ptr)) {
#ifdef SURGE_DEBUG_MEMORY
    log_all<log_event::warning>(
        "In allocator \"{}\"(free): Block pointed by address {:#x} is not the last block. Ignring "
        "free request.",
        allocator_debug_name, uint_ptr);
#endif
    return;
  }

#ifdef SURGE_DEBUG_MEMORY
  log_all<log_event::memory>("Allocator \"{}\" released address {:#x}", allocator_debug_name,
                             // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                             reinterpret_cast<std::uintptr_t>(ptr));
#endif

  const auto [header_idx, data_idx] = ptr_to_idx(ptr);
  free_index = header_idx;
  allocation_counter--;
}

[[nodiscard]] auto surge::stack_allocator::malloc(std::size_t size) noexcept -> void * {
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

[[nodiscard]] auto surge::stack_allocator::calloc(std::size_t num, std::size_t size) noexcept
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

auto surge::stack_allocator::realloc(void *ptr, std::size_t new_size) noexcept -> void * {
#ifdef SURGE_DEBUG_MEMORY
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto uint_ptr = reinterpret_cast<std::uintptr_t>(ptr);
#endif

  if (ptr == nullptr) {
    return this->malloc(new_size);
  }

  if (!is_valid(ptr)) {
#ifdef SURGE_DEBUG_MEMORY
    log_all<log_event::warning>("In allocator \"{}\"(realloc): Block pointed by address {:#x} is "
                                "invalid. Ignoring realloc request",
                                allocator_debug_name, uint_ptr);
#endif
    return nullptr;
  }

  const auto alligned_new_size{align_alloc_size(new_size)};

  if (is_last_block(ptr)) {
    const auto data_idx{std::get<1>(ptr_to_idx(ptr))};

    const auto block_size{free_index - data_idx};

    if (alligned_new_size > block_size) {
      free_index += (alligned_new_size - block_size);
    } else if (alligned_new_size < block_size) {
      free_index -= (block_size - alligned_new_size);
    }

#ifdef SURGE_DEBUG_MEMORY
    log_all<log_event::memory>("Allocator \"{}\" reallocation summary:\n"
                               "  New allocated size {}\n"
                               "  Internal range ({},{})\n"
                               "  Alloction count {}\n"
                               "  RAM address {:#x}",
                               allocator_debug_name, alligned_new_size, data_idx, free_index - 1,
                               allocation_counter,
                               // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                               reinterpret_cast<std::uintptr_t>(ptr));
#endif

    return ptr;

  } else {
#ifdef SURGE_DEBUG_MEMORY
    log_all<log_event::warning>("In allocator \"{}\"(realloc): Block pointed by address {:#x} is "
                                "not the last block. Allocating new memory.",
                                allocator_debug_name, uint_ptr);
#endif

    void *new_block{malloc(alligned_new_size)};
    return new_block;
  }
}

void surge::stack_allocator::save() noexcept {
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

void surge::stack_allocator::restore() noexcept {
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