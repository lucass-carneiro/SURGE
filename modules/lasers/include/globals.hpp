#ifndef SURGE_LASERS_GLOBALS
#define SURGE_LASERS_GLOBALS

#include "renderer.hpp"

#include <glm/glm.hpp>

namespace surge::mod::lasers::globals {

void make_projection(float ww, float wh) noexcept;

auto get_projection() noexcept -> const glm::mat4 &;
auto get_view() noexcept -> const glm::mat4 &;

auto make_line_ctx() noexcept -> std::uint32_t;
auto get_line_ctx() noexcept -> const surge::renderer::line::context &;

} // namespace surge::mod::lasers::globals

#endif // SURGE_LASERS_GLOBALS