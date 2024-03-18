#ifndef SURGE_PTBA_HPP
#define SURGE_PTBA_HPP

#include "gl_includes.hpp"
#include "integer_types.hpp"
#include "logging.hpp"

#include <cstring>
#include <utility>

namespace surge::atom::buffers {

// https://ferransole.wordpress.com/2014/06/08/persistent-mapped-buffers/
// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBufferStorage.xhtml
// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glClientWaitSync.xhtml
// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glMapBufferRange.xhtml
// https://gdcvault.com/play/1020791/
// https://www.slideshare.net/CassEveritt/approaching-zero-driver-overhead (85)

// Persistant triple buffered array
template <typename T> class ptba {
public:
  ptba(GLsizeiptr c, const char *n = "ptba") noexcept
      : name{n},
        capacity{c},
        element_size{sizeof(T)},
        single_buffer_size{capacity * element_size},
        total_size{single_buffer_size * 3} {
    if (!glIsBuffer(handle)) {
      create();
    }
  }

  ptba(const ptba &) = delete;
  ptba(ptba &&) = delete;

  auto operator=(const ptba &) noexcept -> ptba & = delete;
  auto operator=(ptba &&) noexcept -> ptba & = delete;

  ~ptba() noexcept {
    if (glIsBuffer(handle)) {
      destroy();
    }
  }

  void resize(GLsizeiptr c) noexcept {
    destroy();
    capacity = c;
    single_buffer_size = capacity * element_size;
    total_size = single_buffer_size * 3;
    create();
  }

  /*void push(std::same_as<T> auto &&...elements) noexcept {
  using std::memcpy;

  for (usize i = 0; const auto &e : std::initializer_list<T>{elements...}) {
    // get write size
    const GLintptr write_size{write_head + element_size};

    // Check if write size is ilegal and early quit
    const auto write_size_legal{write_size < single_buffer_size};

    if (!write_size_legal) {
      log_warn("PTBA %s: Element %lu in push operation is too big to be written to the buffer. "
               "Ignoring request",
               name, i);
      return;
    }

    // Write is legal. Wait until the GPU usage lock is released. If the region has no locks, it
    // proceeds imediatly
    wait_for_locked_range(write_head, write_size);

    // Write and advance head
    memcpy(data, static_cast<void *>(&e), write_size);
    write_head += write_size;
    write_head %= total_size;

    i++;
  }*/

  /*void bind_to_slot(GLuint shader_slot) {
    // Find wich buffer owns the current head

    // glBindBufferRange(GL_SHADER_STORAGE_BUFFER, shader_slot, handle, write_head, )
  }*/

private:
  const char *name{nullptr};        // Name of the buffer
  GLsizeiptr capacity{0};           // Number of elements in the buffer
  const GLsizeiptr element_size{0}; // Size of a single element in B
  GLsizeiptr single_buffer_size{0}; // Size of a single buffer in B
  GLsizeiptr total_size{0};         // Total buffer size, including repetitions in B

  GLuint handle{0};
  void *map{nullptr};

  struct buffer_view {
    GLintptr start{0};
    GLsizeiptr size{0};
  };

  struct buffer_vector_adapter {
    buffer_view view{};
    GLsizeiptr current_size{0};
  };

  struct range_lock {
    buffer_view range{};
    GLsync lock{nullptr};
  };

  buffer_vector_adapter b_vec_0{};
  buffer_vector_adapter b_vec_1{};
  buffer_vector_adapter b_vec_2{};

  range_lock locked_range{};

  void create() noexcept {
    log_info("Creating PTBA %s\n"
             "  Buffer size: %lu\n"
             "  Element size: %lu\n"
             "  Total size: %lu",
             name, capacity, element_size, total_size);

    const GLbitfield map_flags{GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT};
    const GLbitfield alloc_flags{map_flags | GL_DYNAMIC_STORAGE_BIT};

    glCreateBuffers(1, &handle);
    glNamedBufferStorage(handle, total_size, nullptr, alloc_flags);
    map = glMapNamedBufferRange(handle, 0, total_size, map_flags);

    b_vec_0 = buffer_vector_adapter{buffer_view{0, single_buffer_size}, 0};
    b_vec_1 = buffer_vector_adapter{buffer_view{single_buffer_size, single_buffer_size}, 0};
    b_vec_2 = buffer_vector_adapter{buffer_view{2 * single_buffer_size, single_buffer_size}, 0};
  }

  void destroy() noexcept {
    log_info("Destroying PTBA %s", name);
    glUnmapNamedBuffer(handle);
    glDeleteBuffers(1, &handle);
    handle = 0;
    map = nullptr;
    b_vec_0 = buffer_vector_adapter{};
    b_vec_1 = buffer_vector_adapter{};
    b_vec_2 = buffer_vector_adapter{};
  }

  void lock_range(buffer_view &&bv) noexcept {
    if (locked_range.lock) {
      glDeleteSync(locked_range.lock);
    }
    locked_range.lock = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    locked_range.range = std::move(bv);
  }

  void wait_if_range_locked(const buffer_view &bv) noexcept {
    const auto lock_cnd{locked_range.lock && locked_range.range.start == bv.start
                        && locked_range.range.size == bv.size};
    if (lock_cnd) {
      while (true) {
        const auto wait_return = glClientWaitSync(locked_range.lock, GL_SYNC_FLUSH_COMMANDS_BIT, 1);
        if (wait_return == GL_ALREADY_SIGNALED || wait_return == GL_CONDITION_SATISFIED)
          return;
      }
    }
  }

  void push_element(usize id, const T &element, const buffer_vector_adapter &bvec) {
    using std::memcpy;

    // get size after write
    const auto size_after_write{bvec.view.start + bvec.current_size + element_size};

    // Check if write size is ilegal and early quit
    const auto write_size_legal{size_after_write <= bvec.view.size};

    if (!write_size_legal) {
      log_warn("PTBA %s: Element %lu in push operation is too big to be written to the buffer. "
               "Ignoring request",
               name, id);
      return;
    }

    // Write is legal. Wait until the GPU usage lock is released. If the region has no locks, it
    // proceeds imediatly
    wait_if_range_locked(bvec.view);

    // Write and advance counters
    memcpy(map + bvec.view.start + bvec.current_size, static_cast<void *>(&element), element_size);
    bvec.current_size += element_size;
    bvec.current_size %= single_buffer_size;
  }
};

} // namespace surge::atom::buffers

#endif // SURGE_PTBA_HPP