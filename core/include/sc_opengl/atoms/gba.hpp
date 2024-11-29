#ifndef SURGE_CORE_GL_ATOM_GBA_HPP
#define SURGE_CORE_GL_ATOM_GBA_HPP

#include "integer_types.hpp"
#include "logging.hpp"
#include "options.hpp"
#include "renderer_gl.hpp"

#include <array>

#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#  include <tracy/TracyOpenGL.hpp>
#endif

namespace surge::gl_atom {

/**
 * @brief GBA stands for GPU Bump Array.
 * This is an array of contiguous data that lieves on the GPU.
 *
 * It is a "bump" array in the sense that one cannot remove individual itens from it,
 * it is only possible to "reset" the whole array at once.
 *
 * Reseting DOES NOT mean deallocating the GPU memory holding the data. The memory is instead
 * reused for newer insertions. This is usefull for storing data in the GPU during a frame
 * and resetting the contents when beggining the next frame.
 *
 * Creation and destruction are actual GPU "malloc" calls.
 * Users have the ability to choose when that happens.
 *
 * References:
 * https://www.khronos.org/opengl/wiki/Buffer_Object_Streaming#Persistent_mapped_streaming
 * https://www.slideshare.net/CassEveritt/approaching-zero-driver-overhead
 */
template <typename T, usize redundancy = 3> struct gba {
private:
  usize capacity{0};
  usize write_idx{0};

  usize write_buffer{0};

  std::array<GLuint, redundancy> IDs{};
  std::array<T *, redundancy> buffers{};
  std::array<GLsync, redundancy> syncs{};

#ifdef SURGE_BUILD_TYPE_Debug
  const char *name{"GBA"};
#endif

  void wait_buffer(usize buffer_idx) noexcept {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
    ZoneScopedN("surge::gba::wait_buffer");
    TracyGpuZone("GPU surge::gba::wait_buffer");
#endif
    auto &buffer_sync{syncs[buffer_idx]};
    if (buffer_sync != nullptr) {
      while (true) {
        const auto wait_res{glClientWaitSync(buffer_sync, GL_SYNC_FLUSH_COMMANDS_BIT, 1)};
        if (wait_res == GL_ALREADY_SIGNALED || wait_res == GL_CONDITION_SATISFIED) {
          glDeleteSync(buffer_sync);
          buffer_sync = nullptr;
          return;
        }
      }
    }
  }

public:
  static auto create(usize cap, [[maybe_unused]] const char *name = "GBA") noexcept -> gba {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
    ZoneScopedN("surge::gba::create");
    TracyGpuZone("GPU surge::gba::create");
#endif

    gba<T> gba{};

    gba.IDs.fill(0);
    gba.buffers.fill(nullptr);
    gba.syncs.fill(nullptr);

    gba.capacity = cap;

    const auto total_buffer_size{static_cast<GLsizeiptr>(gba.capacity * sizeof(T))};

    constexpr GLbitfield map_flags{GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT};
    constexpr GLbitfield create_flags{map_flags | GL_DYNAMIC_STORAGE_BIT};

    glCreateBuffers(redundancy, gba.IDs.data());

    for (usize i = 0; i < redundancy; i++) {
      glNamedBufferStorage(gba.IDs[i], total_buffer_size, nullptr, create_flags);
      gba.buffers[i]
          = static_cast<T *>(glMapNamedBufferRange(gba.IDs[i], 0, total_buffer_size, map_flags));
    }

#ifdef SURGE_BUILD_TYPE_Debug
    gba.name = name;
    log_info("Creating GBA \"{}\" with size {} B redundancy {}, totaling {} B", name,
             total_buffer_size, redundancy, redundancy * static_cast<usize>(total_buffer_size));
#endif

    return gba;
  }

  void destroy() noexcept {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
    ZoneScopedN("surge::gba::destroy");
    TracyGpuZone("GPU surge::gba::destroy");
#endif

#ifdef SURGE_BUILD_TYPE_Debug
    log_info("Destroying GBA \"{}\"", name);
#endif
    wait_idle();

    for (const auto &ID : IDs) {
      glUnmapNamedBuffer(ID);
    }

    glDeleteBuffers(redundancy, IDs.data());
  }

  void push(const T &value) noexcept {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
    ZoneScopedN("surge::gba::push");
#endif

#ifdef SURGE_BUILD_TYPE_Debug
    if (write_idx == capacity) {
      log_warn("Unable to add element to GBA {}: Capacity reached. Ignoring push request", name);
      return;
    }
#endif

    wait_buffer(write_buffer);

    buffers[write_buffer][write_idx] = value;
    write_idx++;
  }

  auto get_elm_ptr(usize idx) -> T * {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
    ZoneScopedN("surge::gba::get_elm_ptr");
#endif

#ifdef SURGE_BUILD_TYPE_Debug
    if (idx >= size()) {
      log_warn("Unable to edit element {} in GBA {}:", idx, name);
      return nullptr;
    }
#endif

    return &(buffers[write_buffer][idx]);
  }

  void bind(GLenum target, GLuint location) noexcept {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
    ZoneScopedN("surge::gba::bind");
    TracyGpuZone("GPU surge::gba::bind");
#endif

    const auto buffer_size{static_cast<GLsizeiptr>(write_idx * sizeof(T))};
    glBindBufferRange(target, location, IDs[write_buffer], 0, buffer_size);
  }

  void lock_write_buffer() noexcept {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
    ZoneScopedN("surge::gba::lock_write_buffer");
    TracyGpuZone("GPU surge::gba::lock_write_buffer");
#endif
    auto &buffer_sync{syncs[write_buffer]};
    if (buffer_sync == nullptr) {
      buffer_sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
      write_buffer++;
      write_buffer %= redundancy;
    }
  }

  void reset() noexcept { write_idx = 0; }

  void reinit() noexcept {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
    ZoneScopedN("surge::gba::reinit");
    TracyGpuZone("GPU surge::gba::reinit");
#endif
    wait_idle();
    write_buffer = 0;
    write_idx = 0;
  }

  void wait_idle() noexcept {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
    ZoneScopedN("surge::gba::wait_idle");
    TracyGpuZone("GPU surge::gba::wait_idle");
#endif
    for (usize i = 0; i < redundancy; i++) {
      wait_buffer(i);
    }
  }

  [[nodiscard]] auto size() const noexcept -> usize { return write_idx; }

#ifdef SURGE_BUILD_TYPE_Debug
  [[nodiscard]] auto get_write_buffer() const noexcept -> usize { return write_buffer; }
#endif
};

} // namespace surge::gl_atom

#endif // SURGE_CORE_GL_ATOM_GBA_HPP