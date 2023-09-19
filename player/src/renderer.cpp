#include "renderer.hpp"

void surge::renderer::enable(const capability cap) noexcept { glEnable(static_cast<GLenum>(cap)); }

void surge::renderer::disable(const capability cap) noexcept {
  glDisable(static_cast<GLenum>(cap));
}

void surge::renderer::blend_function(const blend_src src, const blend_dest dest) noexcept {
  glBlendFunc(static_cast<GLenum>(src), static_cast<GLenum>(dest));
}

void surge::renderer::clear(const config::clear_color &ccl) noexcept {
  glClearColor(ccl.r, ccl.g, ccl.b, ccl.a);
  glClear(GL_COLOR_BUFFER_BIT);
  glClear(GL_DEPTH_BUFFER_BIT);
}