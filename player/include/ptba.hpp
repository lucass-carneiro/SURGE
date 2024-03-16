#ifndef SURGE_PTBA_HPP
#define SURGE_PTBA_HPP

#include "gl_includes.hpp"
#include "integer_types.hpp"
#include "logging.hpp"

#include <span>

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
template <typename T, usize num_buffers = 3> class ptba {
public:
  ptba(usize capacity) {
    const auto total_size{capacity * sizeof(T) * num_buffers};

    const GLbitfield map_flags{GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT};
    const GLbitfield alloc_flags{map_flags | GL_DYNAMIC_STORAGE_BIT};

    glCreateBuffers(1, &handle);
    glNamedBufferStorage(handle, total_size, nullptr, alloc_flags);
    data = std::span{
        static_cast<std::byte *>(glMapNamedBufferRange(handle, 0, total_size, map_flags)),
        total_size};
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

  void wait_for_locked_range() noexcept {
    if (range_lock.lock) {
      while (true) {
        const auto wait_return = glClientWaitSync(range_lock.lock, GL_SYNC_FLUSH_COMMANDS_BIT, 1);
        if (wait_return == GL_ALREADY_SIGNALED || wait_return == GL_CONDITION_SATISFIED)
          return;
      }
    }
  }

  locked_range range_lock{};
  usize write_head{0};

  GLuint handle{0};
  std::span<std::byte> data;
};

} // namespace surge::atom::buffers

#endif // SURGE_PTBA_HPP