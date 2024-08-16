#ifndef SURGE_CORE_RENDERER_GL_HPP
#define SURGE_CORE_RENDERER_GL_HPP

#include "config.hpp"
#include "error_types.hpp"
#include "glfw_includes.hpp"

#include <optional>

namespace surge::renderer::gl {

auto init(const config::renderer_attrs &r_attrs) noexcept -> std::optional<error>;
void wait_idle() noexcept;
void clear(const config::clear_color &w_ccl) noexcept;

} // namespace surge::renderer::gl

#endif // SURGE_CORE_RENDERER_GL_HPP