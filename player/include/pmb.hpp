// This is hard to do right and it is currently broken. Maybe try to do it someday?
// https://www.khronos.org/opengl/wiki/Buffer_Object_Streaming#Persistent_mapped_streaming
// https://www.gdcvault.com/play/1020791/Approaching-Zero-Driver-Overhead-in
// (85) https://www.slideshare.net/CassEveritt/approaching-zero-driver-overhead
// https://ferransole.wordpress.com/2014/06/08/persistent-mapped-buffers/

#ifndef SURGE_PMB_HPP
#define SURGE_PMB_HPP

#include "gl_includes.hpp"
#include "integer_types.hpp"
#include "logging.hpp"

#include <cstring>

namespace surge::atom::pmb {

struct buffer_view {
  GLintptr offset{0};
  GLsizeiptr size{0};
};

template <typename T> struct buffer {
private:
  const char *name{nullptr};

  GLsizeiptr max_elements_per_region{0};

  GLuint id{0};
  void *map{nullptr};

  buffer_view region_0{};
  buffer_view region_1{};
  buffer_view region_2{};

  GLsync lock_0{nullptr};
  GLsync lock_1{nullptr};
  GLsync lock_2{nullptr};

  u8 write_region{0};
  GLintptr write_head{0};

  [[nodiscard]] auto get_write_region_view() noexcept -> buffer_view & {
    switch (write_region) {
    case 0:
      return region_0;
    case 1:
      return region_1;
    case 2:
      return region_2;
    default:
      log_warn(
          "PMB %s: Unknown write region. Returning region 0. This may cause CPU-GPU sync. issues.",
          name);
      return region_0;
    }
  }

  [[nodiscard]] auto get_write_region_lock() noexcept -> GLsync & {
    switch (write_region) {
    case 0:
      return lock_0;
    case 1:
      return lock_1;
    case 2:
      return lock_2;
    default:
      log_warn("PMB %s: Unknown write region. Returning lock for region 0. This may cause CPU-GPU "
               "sync. issues.",
               name);
      return lock_0;
    }
  }

  void wait_lock(GLsync &lock) noexcept {
    if (lock) {
      while (true) {
        const auto wait_return{glClientWaitSync(lock, GL_SYNC_FLUSH_COMMANDS_BIT, 1)};
        if (wait_return == GL_ALREADY_SIGNALED || wait_return == GL_CONDITION_SATISFIED)
          return;
      }
    }
  }

public:
  static auto create(GLsizeiptr max_elements, const char *name = "PMB") noexcept -> buffer<T> {
    buffer<T> b{};

    b.name = name;

    b.max_elements_per_region = max_elements;
    const GLsizeiptr element_size{max_elements * static_cast<GLsizeiptr>(sizeof(T))};
    const GLsizeiptr total_size{element_size * 3};

    GLbitfield map_flags{GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT};
    GLbitfield create_falgs{map_flags | GL_DYNAMIC_STORAGE_BIT};

    glCreateBuffers(1, &b.id);
    glNamedBufferStorage(b.id, total_size, nullptr, create_falgs);
    b.map = glMapNamedBufferRange(b.id, 0, total_size, map_flags);

    b.region_0 = buffer_view{0, element_size};
    b.region_1 = buffer_view{element_size, element_size};
    b.region_2 = buffer_view{2 * element_size, element_size};

    return b;
  }

  void destroy() noexcept {
    glUnmapNamedBuffer(id);
    glDeleteBuffers(1, &id);
  }

  void add(T &value) noexcept {
    using std::memcpy;

    // 1. Get region we are supposed to write to
    auto &write_region_view{get_write_region_view()};
    auto &write_region_lock{get_write_region_lock()};

    // 2. Get write start address and write size
    const auto write_offset{write_region_view.offset + write_head};
    constexpr GLsizeiptr write_size{sizeof(T)};

    // 3. Check if the write fits in the current buffer and bail out if not
    if ((write_offset + write_size) > write_region_view.size) {
      log_warn("PMB %s: No space available to write the element in the buffer. Ignoring request.",
               name);
      return;
    }

    // 4. Wait for locks in the region
    wait_lock(write_region_lock);

    // 5. Write the data
    auto *dest{reinterpret_cast<void *>(reinterpret_cast<GLintptr>(map) + write_offset)}; // NOLINT
    auto *src{static_cast<void *>(&value)};
    memcpy(dest, src, write_size);
    write_head += write_size;
  }

  void reset() noexcept {
    switch (write_region) {
    case 0:
      write_head = region_0.offset;
      return;
    case 1:
      write_head = region_1.offset;
      return;
    case 2:
      write_head = region_2.offset;
      return;
    default:
      log_warn("PMB %s: Unknown write region. Setting head for region 0. This may cause CPU-GPU "
               "sync. issues.",
               name);
      write_head = region_0.offset;
      return;
    }
  }

  void bind_to_location(GLenum target, GLuint location) noexcept {
    auto &write_region_view{get_write_region_view()};

    glBindBufferRange(target, location, id, write_region_view.offset,
                      write_head - static_cast<GLsizeiptr>(write_region_view.offset));
  }

  void lock_write_region() noexcept {
    auto &write_region_lock{get_write_region_lock()};

    if (write_region_lock) {
      glDeleteSync(write_region_lock);
    }

    write_region_lock = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    write_region += 1;
    write_region %= 3;

    reset();
  }
};

} // namespace surge::atom::pmb

#endif // SURGE_PMB_HPP