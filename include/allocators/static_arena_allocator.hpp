#ifndef SURGE_STATIC_ARENA_ALLOCATOR_HPP
#define SURGE_STATIC_ARENA_ALLOCATOR_HPP

#include "allocator_utils.hpp"
#include "log.hpp"
#include "options.hpp"

#include <array>

namespace surge {

/**
 * @brief Saves the state of the allocator for use in rewinding
 *
 */
struct static_arena_allocator_state {
  std::size_t allocation_counter{0};
  std::size_t free_index{0};
};

template <std::size_t actual_arena_capacity> class static_arena_allocator {
public:
#ifdef SURGE_DEBUG_MEMORY
  static_arena_allocator(const char *name) : allocator_debug_name{name} {}
#else
  static_arena_allocator(const char *) = default;
#endif

  [[nodiscard]] auto aligned_alloc(std::size_t alignment, std::size_t size) noexcept -> void * {
    // Precondition 1: alignment must be a power of 2
    if (!is_pow_2(alignment)) {
#ifdef SURGE_DEBUG_MEMORY
      glog<log_event::error>("In allocator \"{}\"(aligned_alloc): Allocation of alignment {} "
                             "requested, which is not a power of 2",
                             allocator_debug_name, alignment);
#endif
      return nullptr;
    }

    // Precondition 2: alignment is a multiple of sizeof(void*)
    if ((alignment % sizeof(void *)) != 0) {
#ifdef SURGE_DEBUG_MEMORY
      glog<log_event::error>("In allocator \"{}\"(aligned_alloc): Allocation of alignment {} "
                             "requested, which is not a multiple of sizeof(void*) ({})",
                             allocator_debug_name, alignment, sizeof(void *));
#endif
      return nullptr;
    }

    // Precondition 3: size is a non zero integral multiple of alignment
    if (size == 0 || (size % alignment != 0)) {
#ifdef SURGE_DEBUG_MEMORY
      glog<log_event::error>("In allocator \"{}\"(aligned_alloc): Allocation of size {} "
                             "requested, "
                             "which is not a non zero integral muliple of the alignment {}",
                             allocator_debug_name, size, alignment);
#endif
      return nullptr;
    }

    const auto current_index = free_index;
    const auto bumped_index = current_index + size;

    if (bumped_index >= actual_arena_capacity) {
#ifdef SURGE_DEBUG_MEMORY
      glog<log_event::error>("In allocator \"{}\"(aligned_alloc): Allocation of size {} requested "
                             "exceeds the arena capacity of (actual, remaining) ({}, {}) Bytes",
                             allocator_debug_name, size, actual_arena_capacity,
                             actual_arena_capacity - free_index);
#endif
      return nullptr;
    }

    // Allocation succesfull
    free_index = bumped_index;
    auto start_ptr{&(arena_buffer[current_index])};

#ifdef SURGE_DEBUG_MEMORY
    glog<log_event::memory>("Allocator \"{}\" allocation summary:\n"
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

  [[nodiscard]] auto malloc(std::size_t size) noexcept -> void * {

    // Precondition 1: size must be non null
    if (size == 0) {
#ifdef SURGE_DEBUG_MEMORY
      glog<log_event::error>("In allocator \"{}\"(malloc): Allocation of size 0 is ill defined",
                             allocator_debug_name);
#endif
      return nullptr;
    }

    constexpr const std::size_t default_alignment = alignof(std::max_align_t);
    const auto actual_alloc_size = align_alloc_size(size, default_alignment);

    return this->aligned_alloc(default_alignment, actual_alloc_size);
  }

  [[nodiscard]] auto calloc(std::size_t num, std::size_t size) noexcept -> void * {

    // Precondition 1: num must be non null
    if (num == 0) {
#ifdef SURGE_DEBUG_MEMORY
      glog<log_event::error>("In allocator \"{}\"(calloc): Allocation of 0 elements ill defined",
                             allocator_debug_name);
#endif

      return nullptr;
    }

    // Precondition 2: size must be non null
    if (size == 0) {
#ifdef SURGE_DEBUG_MEMORY
      glog<log_event::error>("In allocator \"{}\"(calloc): Allocation of size "
                             "0 elements ill defined",
                             allocator_debug_name);
#endif

      return nullptr;
    }

    constexpr const std::size_t default_alignment = alignof(std::max_align_t);
    const auto intended_alloc_size{num * size};
    const auto actual_alloc_size = align_alloc_size(intended_alloc_size, default_alignment);

    auto memory = this->aligned_alloc(default_alignment, actual_alloc_size);

    if (memory != nullptr) {
      std::memset(memory, 0, actual_alloc_size);
    }

    return memory;
  }

  [[nodiscard]] auto realloc(void *ptr, std::size_t new_size) noexcept -> void * {
    this->free(ptr);
    return this->malloc(new_size);
  }

  void free(void *ptr) noexcept {
#ifdef SURGE_DEBUG_MEMORY
    glog<log_event::memory>("Allocator \"{}\" released address {:#x}", allocator_debug_name,
                            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                            reinterpret_cast<std::uintptr_t>(ptr));

    allocation_counter--;
#endif
  }

  void reset() noexcept {
    free_index = 0;
    allocation_counter = 0;
  }

  [[nodiscard]] auto save() const noexcept -> static_arena_allocator_state {
#ifdef SURGE_DEBUG_MEMORY
    glog<log_event::memory>("Allocator \"{}\": Saving state:\n"
                            "  free index: {}\n"
                            "  allocation counter: {}",
                            allocator_debug_name, free_index, allocation_counter);
    return static_arena_allocator_state{allocation_counter, free_index};
#endif
    return static_arena_allocator_state{0, free_index};
  }

  void restore(const static_arena_allocator_state &saved_state) noexcept {
#ifdef SURGE_DEBUG_MEMORY
    glog<log_event::memory>("Allocator \"{}\": Restoring state:\n"
                            "  free index: {}\n"
                            "  allocation counter: {}",
                            allocator_debug_name, free_index, allocation_counter);
    allocation_counter = saved_state.allocation_counter;
#endif
    free_index = saved_state.free_index;
  }

#ifdef SURGE_DEBUG_MEMORY
  /**
   * Returns the debug name of the arena.
   * @return The debug name of the arena.
   */
  [[nodiscard]] constexpr auto get_debug_name() const noexcept -> const char * {
    return allocator_debug_name;
  }
#endif

  /**
   * @brief Get the total arena capacity.
   *
   * @return The actual arena capacity after alignment
   */
  [[nodiscard]] constexpr auto get_actual_capacity() const noexcept -> std::size_t {
    return actual_arena_capacity;
  }

private:
#ifdef SURGE_DEBUG_MEMORY
  const char *allocator_debug_name{"static linear arena allocator"};
  std::size_t allocation_counter{0};
#endif

  /**
   * Index to the first free adress in the underlying memory buffer
   */
  std::size_t free_index{0};

  /**
   * @brief Underlying stack memory buffer
   *
   */
  std::array<std::byte, actual_arena_capacity> arena_buffer{};
};

} // namespace surge

#endif // SURGE_STATIC_ARENA_ALLOCATOR_HPP