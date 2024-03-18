#ifndef SURGE_PTBA_HPP
#define SURGE_PTBA_HPP

#include "gl_includes.hpp"
#include "integer_types.hpp"
#include "logging.hpp"

#include <concepts>
#include <cstring>
#include <initializer_list>

namespace surge::atom::buffers {

// https://ferransole.wordpress.com/2014/06/08/persistent-mapped-buffers/
// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBufferStorage.xhtml
// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glClientWaitSync.xhtml
// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glMapBufferRange.xhtml
// https://gdcvault.com/play/1020791/
// https://www.slideshare.net/CassEveritt/approaching-zero-driver-overhead (85)

struct locked_range {
  usize write_start{0};
  usize write_size{0};
  GLsync lock{nullptr};
};

// Persistant triple buffered array
template <typename T> class ptba {
public:
  ptba(usize c, const char *n = "ptba", usize r = 3) noexcept
      : capacity{c},
        name{n},
        redundancy{r},
        element_size{sizeof(T)},
        single_buffer_size{capacity * element_size},
        total_size{single_buffer_size * redundancy},
        bound_buffer{redundancy + 1} {

    log_info("Creating PTBA %s"
             "  Buffer size: %u"
             "  Element size: %u"
             "  Buffering redundancy: %u"
             "  Total size: %u",
             name, capacity, element_size, redundancy, total_size);

    const GLbitfield map_flags{GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT};
    const GLbitfield alloc_flags{map_flags | GL_DYNAMIC_STORAGE_BIT};

    glCreateBuffers(1, &handle);
    glNamedBufferStorage(handle, total_size, nullptr, alloc_flags);
    data = glMapNamedBufferRange(handle, 0, total_size, map_flags);
    write_head = reinterpret_cast<uintptr_t>(data); // NOLINT
  }

  void destroy() noexcept {
    log_info("Destroying PTBA %s", name);
    glUnmapNamedBuffer(handle);
    glDeleteBuffers(1, &handle);
    handle = 0;
  };

  ~ptba() noexcept {
    if (glIsBuffer(handle)) {
      destroy();
    }
  }

  void push(std::same_as<T> auto &&...elements) noexcept {
    using std::memcpy;

    for (usize i = 0; const auto &e : std::initializer_list<T>{elements...}) {
      // get write size
      const usize write_size{write_head + element_size};

      // Check if write size is ilegal and early quit
      const auto write_size_legal{write_size < single_buffer_size};

      if (!write_size_legal) {
        log_warn("PTBA %s: Element %lu in push operation is too big to be written to the buffer. "
                 "Ignoring request",
                 name, i);
        return;
      }

      // Write is legal. Wait until the GPU usage lock is released.
      wait_for_locked_range(write_size);

      // Write and advance head
      memcpy(data, static_cast<void *>(&e), write_size);
      write_head += write_size;
      write_head %= total_size;

      i++;
    }
  }

  void bind_to_slot(GLuint shader_slot) {
    // Find wich buffer owns
    // glBindBufferRange(GL_SHADER_STORAGE_BUFFER, shader_slot, handle, write_head, )
  }

  ptba(const ptba &) = delete;
  ptba(ptba &&) = delete;
  auto operator=(const ptba &) noexcept -> ptba & = delete;
  auto operator=(ptba &&) noexcept -> ptba & = delete;

private:
  /*
  after writing/drawing(?)place fence that knows location and size of write
  a few frames later when trying to write to the same region, wait for lock
  does the region i'm about to write overlaps with a region prev. written? If so
  */
  void lock_range(usize start, usize size) noexcept {
    if (range_lock.lock) {
      glDeleteSync(range_lock.lock);
    }
    range_lock.lock = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    range_lock.write_start = start;
    range_lock.write_size = size;
  }

  void wait_for_locked_range(usize) noexcept {
    if (range_lock.lock) {
      while (true) {
        const auto wait_return = glClientWaitSync(range_lock.lock, GL_SYNC_FLUSH_COMMANDS_BIT, 1);
        if (wait_return == GL_ALREADY_SIGNALED || wait_return == GL_CONDITION_SATISFIED)
          return;
      }
    }
  }

  void get_head_owner() noexcept {}

  const usize capacity{0};           // Number of elements in the buffer
  const char *name{nullptr};         // Name of the buffer
  const usize redundancy{0};         // Number of repetitions (double, triple, etc buffering)
  const usize element_size{0};       // Size of a single element in B
  const usize single_buffer_size{0}; // Size of a single buffer in B
  const usize total_size{0};         // Total buffer size, including repetitions in B

  usize bound_buffer{0};
  uintptr_t write_head{0};
  locked_range range_lock{};

  GLuint handle{0};
  void *data{nullptr};
};

} // namespace surge::atom::buffers

#endif // SURGE_PTBA_HPP