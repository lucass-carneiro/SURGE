#ifndef SURGE_CORE_GL_ATOM_PMB_HPP
#define SURGE_CORE_GL_ATOM_PMB_HPP

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
 * References:
 * https://www.khronos.org/opengl/wiki/Buffer_Object_Streaming#Persistent_mapped_streaming
 * https://www.slideshare.net/CassEveritt/approaching-zero-driver-overhead
 **/

} // namespace surge::gl_atom

#endif // SURGE_CORE_GL_ATOM_PMB_HPP