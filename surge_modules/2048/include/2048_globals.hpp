#ifndef SURGE_2048_GLOBALS_HPP
#define SURGE_2048_GLOBALS_HPP

#include <glm/glm.hpp>

namespace mod_2048::globals {

void make_projection(float ww, float wh) noexcept;

auto get_projection() noexcept -> const glm::mat4 &;
auto get_view() noexcept -> const glm::mat4 &;

} // namespace mod_2048::globals

#endif // SURGE_2048_GLOBALS_HPP