#ifndef SURGE_BUFFER_USAGE_HINTS_HPP
#define SURGE_BUFFER_USAGE_HINTS_HPP

#include "headers.hpp"

namespace surge {
/**
 * @brief Wrapper around OpenGL usage hints. See See
 * https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBufferData.xhtml
 *
 */
enum class buffer_usage_hint : GLenum {
  // The data store contents will be modified once and used at most a few times. The data store
  // contents are modified by the application, and used as the source for GL drawing and image
  // specification commands.
  stream_draw = GL_STREAM_DRAW,

  // The data store contents will be modified once and used at most a few times. The data store
  // contents are modified by reading data from the GL, and used to return that data when queried by
  // the application.
  stream_read = GL_STREAM_READ,

  // The data store contents will be modified once and used at most a few times. The data store
  // contents are modified by reading data from the GL, and used as the source for GL drawing and
  // image specification commands.
  stream_copy = GL_STREAM_COPY,

  // The data store contents will be modified once and used many times. The data store contents are
  // modified by the application, and used as the source for GL drawing and image specification
  // commands.
  static_draw = GL_STATIC_DRAW,

  // The data store contents will be modified once and used many times. The data store contents are
  // modified by reading data from the GL, and used to return that data when queried by the
  // application.
  static_read = GL_STATIC_READ,

  // The data store contents will be modified once and used many times. The data store contents are
  // modified by reading data from the GL, and used as the source for GL drawing and image
  // specification commands.
  static_copy = GL_STATIC_COPY,

  // The data store contents will be modified repeatedly and used many times. The data store
  // contents are modified by the application, and used as the source for GL drawing and image
  // specification commands.
  dynamic_draw = GL_DYNAMIC_DRAW,

  // The data store contents will be modified repeatedly and used many times. The data store
  // contents are modified by reading data from the GL, and used to return that data when queried by
  // the application.
  dynamic_read = GL_DYNAMIC_READ,

  // The data store contents will be modified repeatedly and used many times. The data store
  // contents are modified by reading data from the GL, and used as the source for GL drawing and
  // image specification commands.
  dynamic_copy = GL_DYNAMIC_COPY
};

[[nodiscard]] inline constexpr auto to_gl_hint(buffer_usage_hint hint) noexcept -> GLenum {
  return static_cast<GLenum>(hint);
}

} // namespace surge

#endif // SURGE_BUFFER_USAGE_HINTS_HPP