#include "renderer.hpp"

#include "options.hpp"

#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#endif

void surge::renderer::enable(const capability cap) noexcept {
  if (cap == capability::wireframe) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  } else {
    glEnable(static_cast<GLenum>(cap));
  }
}

void surge::renderer::disable(const capability cap) noexcept {
  glDisable(static_cast<GLenum>(cap));
  if (cap == capability::wireframe) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  } else {
    glDisable(static_cast<GLenum>(cap));
  }
}

void surge::renderer::blend_function(const blend_src src, const blend_dest dest) noexcept {
  glBlendFunc(static_cast<GLenum>(src), static_cast<GLenum>(dest));
}

void surge::renderer::clear(const config::clear_color &ccl) noexcept {
#if defined(SURGE_BUILD_TYPE_Profile) && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("surge::renderer::clear");
#endif

  glClearColor(ccl.r, ccl.g, ccl.b, ccl.a);
  glClear(GL_COLOR_BUFFER_BIT);
  glClear(GL_DEPTH_BUFFER_BIT);
}