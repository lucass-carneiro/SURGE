#ifndef SURGE_GL_RING_BUFFER_HPP
#define SURGE_GL_RING_BUFFER_HPP

#include "gl_includes.hpp"
#include "integer_types.hpp"
#include "logging.hpp"

#include <gsl/gsl-lite.hpp>

namespace surge::atom {

// https://www.khronos.org/opengl/wiki/Buffer_Object_Streaming#Persistent_mapped_streaming
// https://www.slideshare.net/CassEveritt/approaching-zero-driver-overhead

template <typename T, usize num_sections = 3> class gl_ring_buffer {
  using sec_idxs_t = std::array<usize, num_sections>;
  using fences_t = std::array<GLsync, num_sections>;

private:
  static constexpr GLbitfield map_flags{GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT
                                        | GL_MAP_COHERENT_BIT};
  static constexpr GLbitfield create_flags{map_flags | GL_DYNAMIC_STORAGE_BIT};

  const char *name{nullptr};

  usize max_elements_per_section{0};
  GLsizeiptr total_buffer_size{0};

  GLuint buffer_ID{0};

  sec_idxs_t section_start_idxs{};
  sec_idxs_t section_end_idxs{};

  fences_t section_fences{};

  gsl::owner<T *> buffer_data{nullptr};

  usize write_section{0};

  auto create_buffer() const noexcept -> GLuint {
    GLuint ID{0};
    glCreateBuffers(1, &ID);
    glNamedBufferStorage(ID, total_buffer_size, nullptr, create_flags);
    return ID;
  }

  auto compute_section_start_idxs() const noexcept -> sec_idxs_t {
    sec_idxs_t sec_start_idxs{};
    for (usize i = 0; i < num_sections; i++) {
      sec_start_idxs[i] = i * max_elements_per_section;
    }

    return sec_start_idxs;
  }

  auto init_fences() const noexcept -> fences_t {
    fences_t fences{};
    for (auto &fence : fences) {
      fence = nullptr;
    }

    return fences;
  }

  auto map_buffer() const noexcept -> T * {
    return static_cast<T *>(glMapNamedBufferRange(buffer_ID, 0, total_buffer_size, map_flags));
  }

  void close_buffer() noexcept {
    glUnmapNamedBuffer(buffer_ID);
    glDeleteBuffers(1, &buffer_ID);
  }

  void wait_section() const noexcept {
    auto &fence{section_fences[write_section]};
    if (fence != nullptr) {
      while (true) {
        const auto wait_res{glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 1)};
        if (wait_res == GL_ALREADY_SIGNALED || wait_res == GL_CONDITION_SATISFIED) {
          return;
        }
      }
    }
  }

  void lock_section() noexcept {
    auto &fence{section_fences[write_section]};
    if (fence != nullptr) {
      glDeleteSync(fence);
      fence = nullptr;
    }
    fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
  }

public:
  gl_ring_buffer(usize max_elements = 32,
                 const char *nm = "OpenGL Persistent Mapped Ring Buffer") noexcept
      : name{nm},
        max_elements_per_section{max_elements},
        total_buffer_size{
            static_cast<GLsizeiptr>(max_elements_per_section * num_sections * sizeof(T))},
        buffer_ID{create_buffer()},
        section_start_idxs{std::move(compute_section_start_idxs())},
        section_end_idxs{std::move(compute_section_start_idxs())},
        section_fences{std::move(init_fences())},
        buffer_data{map_buffer()} {}

  ~gl_ring_buffer() noexcept { close_buffer(); }

  gl_ring_buffer(gl_ring_buffer &&) = default;
  gl_ring_buffer &operator=(gl_ring_buffer &&) = default;

  [[nodiscard]] auto size() const noexcept {
    const auto end_idx{section_end_idxs[write_section] - section_start_idxs[write_section]};
    return end_idx;
  }

  void push_elm(const T &elm) noexcept {
    if (size() > max_elements_per_section) {
      log_warn("%s: Write section %lu is full. Ignoring push request", name, write_section);
      return;
    }

    wait_section();

    auto &write_idx{section_end_idxs[write_section]};
    buffer_data[write_idx] = elm;
    write_idx += 1;
  }

  void bind(GLenum target, GLuint location) noexcept {
    const auto buffer_start_offset{static_cast<GLintptr>(section_start_idxs[write_section])};
    const auto buffer_size{static_cast<GLsizeiptr>(size())};

    glBindBufferRange(target, location, buffer_ID, buffer_start_offset, buffer_size);
  }

  void lock_and_advance() noexcept {
    lock_section();

    section_end_idxs[write_section] = section_start_idxs[write_section];

    if (write_section + 1 >= num_sections) {
      write_section = 0;
    } else {
      write_section += 1;
    }
  }
};

} // namespace surge::atom

#endif // SURGE_GL_RING_BUFFER_HPP