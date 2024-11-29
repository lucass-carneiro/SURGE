#ifndef SURGE_CORE_GL_ATOM_PV_UBO_HPP
#define SURGE_CORE_GL_ATOM_PV_UBO_HPP

#include <glm/glm.hpp>
#include "renderer_gl.hpp"

// UBO for storing projection and view matrices
namespace surge::gl_atom::pv_ubo {

struct buffer {
private:
  GLuint id{0};

public:
  static auto create() noexcept -> buffer;
  void destroy() noexcept;

  void update_all(const glm::mat4 *projection, const glm::mat4 *view) noexcept;
  void update_view(const glm::mat4 *view) noexcept;
  void bind_to_location(GLuint location) noexcept;
};

} // namespace surge::gl_atom::pv_ubo

#endif // SURGE_CORE_GL_ATOM_PV_UBO_HPP