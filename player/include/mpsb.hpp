#ifndef SURGE_ATOM_MPSB_HPP
#define SURGE_ATOM_MPSB_HPP

#include "renderer.hpp"

#include <glm/glm.hpp>

/*
 * MPSB - Matrix pair shader bufer.
 * An SSBO that store two 4x4 matrices. Usefull for storing
 * view and projection matrices in 2D games.
 */

namespace surge::atom::mpsb {

auto create_buffer() noexcept -> GLuint;
void destroy_buffer(GLuint) noexcept;

void send_buffer(GLuint MPSB, glm::mat4 *m1, glm::mat4 *m2) noexcept;
void bind_to_location(GLuint MPSB, GLuint location) noexcept;

} // namespace surge::atom::mpsb

#endif // SURGE_ATOM_MPSB_HPP