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
// https://www.bilibili.com/video/BV1V64y1h7UJ/
// https://www.slideshare.net/CassEveritt/approaching-zero-driver-overhead (85)

struct size_aware_fence {
  usize written_bytes{0};
  GLsync fence{};
};

struct ptba_options {
  usize max_elements{1024};
  usize buffers{3};
};

// Persistant triple buffered array
template <typename T> class ptba {
public:
  // TODO Copy and move
  ptba(const ptba_options &opts)
      : elements_per_buffer{opts.max_elements},
        num_buffers{opts.buffers},
        single_buffer_size{elements_per_buffer * element_size},
        total_size{opts.buffers * single_buffer_size} {

    log_info("Creating PTBA\n"
             "  Elms. per buffer: %lu\n"
             "  Elm. size: %lu B\n"
             "  Num. buffers: %lu B\n"
             "  Total size: %lu B",
             elements_per_buffer, element_size, opts.buffers);

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
  static constexpr auto element_size{sizeof(T)};
  const usize elements_per_buffer{0};
  const usize num_buffers{0};
  const usize single_buffer_size{0};
  const usize total_size{0};

  usize dest_head{0};

  GLuint handle{0};
  std::span<std::byte> data;
};

} // namespace surge::atom::buffers

#endif // SURGE_PTBA_HPP