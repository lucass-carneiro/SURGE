#ifndef SURGE_ATOM_PV_UBO_HPP
#define SURGE_ATOM_PV_UBO_HPP

#include "gl_includes.hpp"

#include <glm/glm.hpp>

// UBO for storing projection and view matrices
namespace surge::atom::pv_ubo {

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

} // namespace surge::atom::pv_ubo

#endif // SURGE_ATOM_PV_UBO_HPP