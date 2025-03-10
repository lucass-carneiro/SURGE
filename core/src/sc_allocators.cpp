#include "sc_allocators.hpp"

#include "sc_logging.hpp"

#include <mimalloc.h>

#ifdef SURGE_ENABLE_TRACY
#  include <tracy/Tracy.hpp>
#endif

#include <mutex>

namespace surge::allocators {

auto mimalloc::malloc(usize size) -> void * {
  auto p{mi_malloc(size)};
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Memory Event\n"
            "---\n"
            "type: alloc\n"
            "allocator: \"mimalloc::malloc\"\n"
            "size: {}\n"
            "address: {}\n"
            "failed: {}",
            size, p, p ? "false" : "true");
#endif
  return p;
}

auto mimalloc::realloc(void *p, usize newsize) -> void * {
  auto q{mi_realloc(p, newsize)};
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Memory Event\n"
            "---\n"
            "type: realloc\n"
            "allocator: \"mimalloc::realloc\"\n"
            "new size: {}\n"
            "old address: {}\n"
            "new address: {}\n"
            "failed: {}",
            newsize, p, q, q ? "false" : "true");
#endif
  return q;
}

auto mimalloc::calloc(usize count, usize size) -> void * {
  auto p{mi_calloc(count, size)};
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Memory Event\n"
            "---\n"
            "type: alloc\n"
            "allocator: \"mimalloc::calloc\"\n"
            "size: {}\n"
            "count: {}\n"
            "address: {}\n"
            "failed: {}",
            size, count, p, p ? "false" : "true");
#endif
  return p;
}

void mimalloc::free(void *p) {
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Memory Event\n"
            "---\n"
            "type: free\n"
            "allocator: \"mimalloc::free\"\n"
            "address: {}",
            p);
#endif
  mi_free(p);
}

auto mimalloc::aligned_alloc(usize size, usize alignment) -> void * {
  auto p{mi_aligned_alloc(alignment, size)};
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Memory Event\n"
            "---\n"
            "type: alloc\n"
            "allocator: \"mimalloc::mi_aligned_alloc\"\n"
            "size: {}\n"
            "alignment: {}\n"
            "address: {}\n"
            "failed: {}",
            size, alignment, p, p ? "false" : "true");
#endif
  return p;
}

auto mimalloc::aligned_realloc(void *p, usize newsize, usize alignment) -> void * {
  auto q{mi_realloc_aligned(p, newsize, alignment)};
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Memory Event\n"
            "---\n"
            "type: realloc\n"
            "allocator: \"mimalloc::mi_realloc_aligned\"\n"
            "new size: {}\n"
            "alignent: {}\n",
            "old address: {}\n"
            "new address: {}\n"
            "failed: {}",
            newsize, alignment, p, q, q ? "false" : "true");
#endif
  return q;
}

void mimalloc::aligned_free(void *p, usize alignment) {
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Memory Event\n"
            "---\n"
            "type: free\n"
            "allocator: \"mimalloc::aligned_free\"\n"
            "address: {}",
            p);
#endif
  mi_free_aligned(p, alignment);
}

void mimalloc::init() {
  // see https://microsoft.github.io/mimalloc/group__options.html
#ifdef SURGE_DEBUG_MEMORY
  mi_option_enable(mi_option_show_errors);
  mi_option_enable(mi_option_show_stats);
  mi_option_enable(mi_option_verbose);
#else
  mi_option_disable(mi_option_show_errors);
  mi_option_disable(mi_option_show_stats);
  mi_option_disable(mi_option_verbose);
#endif

  mi_option_enable(mi_option_eager_commit);
  mi_option_enable(mi_option_disallow_os_alloc);
  mi_option_enable(mi_option_destroy_on_exit);

  mi_option_set(mi_option_reserve_huge_os_pages, 1);
  mi_option_set(mi_option_eager_commit_delay, 100);
}

/**
 * This is a CPU memory arena that grows when needed.
 */
struct dynamic_arena {
  usize current_capacity{0};
  usize current_offset{0};
  const char *arena_name{nullptr};
  std::byte *memory_data{nullptr};
};

static auto dynamic_arena_init(usize initial_size, const char *arena_name)
    -> Result<dynamic_arena> {
  using std::memset;

  log_info("Allocating initial {} B for arena \"{}\"", initial_size, arena_name);

  auto memory_data = static_cast<std::byte *>(mimalloc::malloc(initial_size));

  if (memory_data == nullptr) {
    log_error("Unable to initially allocate {} B for arena \"{}\"", initial_size, arena_name);
    return tl::unexpected{Error::dynamic_arena_init};
  }

  memset(memory_data, 0, initial_size);
  log_info("Arena \"{}\" ready to operate with initial {} B of capacity", arena_name, initial_size);

  return dynamic_arena{.current_capacity = initial_size,
                       .current_offset = 0,
                       .arena_name = arena_name,
                       .memory_data = memory_data};
}

static void dynamic_arena_destroy(dynamic_arena &da) {
  log_info("Deallocating arena \"{}\" with current size {}, current capacity {}", da.arena_name,
           da.current_offset, da.current_capacity);
  mimalloc::free(da.memory_data);
}

static auto is_pow_2(surge::usize x) { return (x & (x - 1)) == 0; }

static auto dynamic_arena_malloc(dynamic_arena &da, usize size, usize alignment) -> Result<void *> {
  using std::memset;

  if (!is_pow_2(alignment)) {
    log_error("{} B allocation on arena \"{}\" failed: Alignment {} is not a power of 2", size,
              da.arena_name, alignment);
    return tl::unexpected{Error::dynamic_arena_alloc};
  }

  const auto modulo{size & (alignment - 1)}; // same as size % alignment when a is a power of 2
  const auto actual_size{size + modulo};
  const auto new_offset{da.current_offset + actual_size};

  if (new_offset >= da.current_capacity) {
    const auto new_capacity{da.current_capacity * 2};

    log_info("Growing arena \"{}\" from {} B to {} B of capacity", da.arena_name,
             da.current_capacity, new_capacity);

    auto new_memory_data{static_cast<std::byte *>(mimalloc::malloc(new_capacity))};

    if (new_memory_data == nullptr) {
      log_error("Failed to grow arena \"{}\"", da.arena_name);
      return tl::unexpected{dynamic_arena_grow};
    }

    memcpy(new_memory_data, da.memory_data, da.current_offset);
    mimalloc::free(da.memory_data);

    da.current_capacity = new_capacity;
    da.memory_data = new_memory_data;
  }

  auto p{static_cast<void *>(&(da.memory_data[da.current_offset]))};
  da.current_offset = new_offset;
  memset(p, 0, actual_size);

#ifdef SURGE_DEBUG_MEMORY
  log_debug("Memory Event\n"
            "---\n"
            "type: alloc\n"
            "allocator: \"{}\"\n "
            "size: {}\n"
            "alignment: {}\n"
            "total aligned size: {}\n"
            "address: {}\n",
            da.arena_name, size, alignment, actual_size, p);
#endif

  return p;
}

static void dynamic_arena_reset(dynamic_arena &da) { da.current_offset = 0; }

/**
 *  Dynamic arenas used in scoped allocations
 */
static dynamic_arena program_scope_dynamic_arena{};
static std::mutex program_scope_dynamic_arena_mutex{};

static dynamic_arena module_scope_dynamic_arena{};
static std::mutex module_scope_dynamic_arena_mutex{};

static dynamic_arena frame_scope_dynamic_arena{};
static std::mutex frame_scope_dynamic_arena_mutex{};

auto scoped::init() -> Result<void> {
  log_info("Initializing scoped CPU memory arenas");

  const auto ps{dynamic_arena_init(1024, "Program scope arena")};
  const auto ms{dynamic_arena_init(1024, "Frame scope arena")};
  const auto fs{dynamic_arena_init(1024, "Module scope arena")};

  if (!ps) {
    log_error("Unable to initialize program scope CPU memory arenas");
    return Err{ps.error()};
  }

  if (!ms) {
    log_error("Unable to initialize frame scope CPU memory arenas");
    return Err{ms.error()};
  }

  if (!fs) {
    log_error("Unable to initialize module scope CPU memory arenas");
    return Err{fs.error()};
  }

  program_scope_dynamic_arena = *ps;
  module_scope_dynamic_arena = *ms;
  frame_scope_dynamic_arena = *fs;

  return {};
}

void scoped::destroy() {
  log_info("Destroying scoped CPU memory arenas");

  dynamic_arena_destroy(frame_scope_dynamic_arena);
  dynamic_arena_destroy(module_scope_dynamic_arena);
  dynamic_arena_destroy(program_scope_dynamic_arena);
}

auto scoped::malloc(const Lifetimes &lifetime, usize size, usize alignment) -> Result<void *> {
  switch (lifetime) {
  case Lifetimes::Program: {
    const std::lock_guard lock{program_scope_dynamic_arena_mutex};
    return dynamic_arena_malloc(program_scope_dynamic_arena, size, alignment);
  }

  case Lifetimes::Module: {
    const std::lock_guard lock{module_scope_dynamic_arena_mutex};
    return dynamic_arena_malloc(module_scope_dynamic_arena, size, alignment);
  }

  case Lifetimes::Frame: {
    const std::lock_guard lock{frame_scope_dynamic_arena_mutex};
    return dynamic_arena_malloc(program_scope_dynamic_arena, size, alignment);
  }
  default:
    log_error("Allocation requested in unrecognized lifetime scope");
    return nullptr;
  }
}

void scoped::reset(const Lifetimes &lifetime) {
  switch (lifetime) {
  case Lifetimes::Program: {
    const std::lock_guard lock{program_scope_dynamic_arena_mutex};
    dynamic_arena_reset(program_scope_dynamic_arena);
    break;
  }

  case Lifetimes::Module: {
    const std::lock_guard lock{module_scope_dynamic_arena_mutex};
    dynamic_arena_reset(module_scope_dynamic_arena);
    break;
  }

  case Lifetimes::Frame: {
    const std::lock_guard lock{frame_scope_dynamic_arena_mutex};
    dynamic_arena_reset(program_scope_dynamic_arena);
    break;
  }
  }
}

} // namespace surge::allocators