#include "gl_atoms/pv_ubo.hpp"

#include "logging.hpp"

#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo)) && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#  include <tracy/TracyOpenGL.hpp>
#endif

auto surge::gl_atom::pv_ubo::buffer::create() noexcept -> buffer {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo)) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::pv_ubo::buffer::create");
  TracyGpuZone("GPU surge::gl_atom::pv_ubo::buffer::create");
#endif
  log_info("Creating Position/View UBO");
  buffer ubo;

  const GLbitfield create_flags{GL_DYNAMIC_STORAGE_BIT};
  const GLsizeiptr buffer_size{sizeof(glm::mat4) * 2};

  glCreateBuffers(1, &ubo.id);
  glNamedBufferStorage(ubo.id, buffer_size, nullptr, create_flags);

  return ubo;
}

void surge::gl_atom::pv_ubo::buffer::destroy() noexcept {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo)) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::pv_ubo::buffer::destroy");
  TracyGpuZone("GPU surge::gl_atom::pv_ubo::buffer::destroy");
#endif
  log_info("Destroying Position/View UBO");
  glDeleteBuffers(1, &id);
}

void surge::gl_atom::pv_ubo::buffer::update_all(const glm::mat4 *projection,
                                                const glm::mat4 *view) noexcept {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo)) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::pv_ubo::buffer::update_all");
  TracyGpuZone("GPU surge::gl_atom::pv_ubo::buffer::update_all");
#endif
  glNamedBufferSubData(id, 0, static_cast<GLsizeiptr>(sizeof(glm::mat4)), projection);
  glNamedBufferSubData(id, static_cast<GLintptr>(sizeof(glm::mat4)),
                       static_cast<GLsizeiptr>(sizeof(glm::mat4)), view);
}

void surge::gl_atom::pv_ubo::buffer::update_view(const glm::mat4 *view) noexcept {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo)) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::pv_ubo::buffer::update_view");
  TracyGpuZone("GPU surge::gl_atom::pv_ubo::buffer::update_view");
#endif
  glNamedBufferSubData(id, static_cast<GLintptr>(sizeof(glm::mat4)),
                       static_cast<GLsizeiptr>(sizeof(glm::mat4)), view);
}

void surge::gl_atom::pv_ubo::buffer::bind_to_location(GLuint location) noexcept {
#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo)) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::gl_atom::pv_ubo::buffer::bind_to_location");
  TracyGpuZone("GPU surge::gl_atom::pv_ubo::buffer::bind_to_location");
#endif

  glBindBufferBase(GL_UNIFORM_BUFFER, location, id);
}