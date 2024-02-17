#include "mpsb.hpp"

#include "logging.hpp"

auto surge::atom::mpsb::create_buffer() noexcept -> GLuint {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::mpsb::create_buffer");
  TracyGpuZone("GPU surge::atom::mpsb::create_buffer");
#endif

  log_info("Creating MPSB");

  GLuint MPSB{0};
  glCreateBuffers(1, &MPSB);
  glNamedBufferStorage(MPSB, static_cast<GLsizeiptr>(sizeof(glm::mat4) * 2), nullptr,
                       GL_DYNAMIC_STORAGE_BIT);

  return MPSB;
}

void surge::atom::mpsb::destroy_buffer(GLuint MPSB) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::mpsb::destroy_buffers");
  TracyGpuZone("GPU surge::atom::mpsb::destroy_buffers");
#endif

  log_info("Deleting MPSB %u", MPSB);

  glDeleteBuffers(1, &MPSB);
}

void surge::atom::mpsb::send_buffer(GLuint MPSB, glm::mat4 *m1, glm::mat4 *m2) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::mpsb::send_buffers");
  TracyGpuZone("GPU surge::atom::mpsb::send_buffers");
#endif
  glNamedBufferSubData(MPSB, 0, static_cast<GLsizeiptr>(sizeof(glm::mat4)), m1);
  glNamedBufferSubData(MPSB, static_cast<GLintptr>(sizeof(glm::mat4)),
                       static_cast<GLsizeiptr>(sizeof(glm::mat4)), m2);
}

void surge::atom::mpsb::bind_to_location(GLuint MPSB, GLuint location) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::atom::mpsb::bind_to_location");
  TracyGpuZone("GPU surge::atom::mpsb::bind_to_location");
#endif

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, location, MPSB);
}