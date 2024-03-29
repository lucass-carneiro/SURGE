#ifndef SURGE_GL_RING_BUFFER_HPP
#define SURGE_GL_RING_BUFFER_HPP

#include "gl_includes.hpp"
#include "integer_types.hpp"
#include "logging.hpp"

#include <array>
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

  const usize max_elements_per_section{0};
  const GLsizeiptr total_buffer_size{0};

  const GLuint buffer_ID{0};

  const sec_idxs_t section_start_idxs{};
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

  void destroy_buffer() const noexcept { glDeleteBuffers(1, &buffer_ID); }

  auto compute_section_start_idxs() const noexcept -> sec_idxs_t {
    sec_idxs_t sec_start_idxs{};
    for (usize i = 0; i < num_sections; i++) {
      sec_start_idxs[i] = i * max_elements_per_section;
    }

    return sec_start_idxs;
  }

  auto compute_section_end_idxs() const noexcept -> sec_idxs_t {
    sec_idxs_t sec_end_idxs{};
    for (auto &idx : sec_end_idxs) {
      idx = 0;
    }

    return sec_end_idxs;
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

  void unmap_buffer() { glUnmapNamedBuffer(buffer_ID); }

  void wait_section() const noexcept {
    auto &fence{section_fences[write_section]};
    if (fence) {
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
    if (fence) {
      glDeleteSync(fence);
    }
    fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
  }

  void advance_write_section() noexcept {
    if (write_section + 1 > num_sections) {
      write_section = 0;
    } else {
      write_section += 1;
    }
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
        section_end_idxs{std::move(compute_section_end_idxs())},
        section_fences{std::move(init_fences())},
        buffer_data{map_buffer()} {}

  ~gl_ring_buffer() noexcept {
    unmap_buffer();
    destroy_buffer();
  }

  [[nodiscard]] auto size() const noexcept {
    const auto end_idx{section_end_idxs[write_section]};
    return end_idx;
  }

  void push_elm(const T &elm) noexcept {
    const auto start_idx{section_start_idxs[write_section]};
    auto &end_idx{section_end_idxs[write_section]};

    if ((end_idx + 1) > max_elements_per_section) {
      log_warn("%s: Write section %lu is full. Ignoring push request", name, write_section);
      return;
    }

    wait_section();

    const auto push_idx{start_idx + end_idx};
    buffer_data[push_idx] = elm;
    end_idx += 1;
  }

  void bind(GLenum target, GLuint location) noexcept {
    const auto start_idx{section_start_idxs[write_section]};

    const auto buffer_start_ptr{static_cast<void *>(&buffer_data[start_idx])};
    const auto buffer_start_offset{reinterpret_cast<GLintptr>(buffer_start_ptr)};

    GLsizeiptr buffer_size{section_end_idxs[write_section]};

    glBindBufferRange(target, location, buffer_ID, buffer_start_offset, buffer_size);
  }

  void lock_and_advance() noexcept {
    lock_section();
    section_end_idxs[write_section] = 0;
    advance_write_section();
  }
};

} // namespace surge::atom

#endif // SURGE_GL_RING_BUFFER_HPP