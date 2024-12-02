#ifndef SURGE_CORE_RENDERER_OPENGL_HPP
#define SURGE_CORE_RENDERER_OPENGL_HPP

#include "sc_config.hpp"
#include "sc_error_types.hpp"
#include "sc_glfw_includes.hpp"

#include <optional>

namespace surge::renderer::gl {

auto init(const config::renderer_attrs &r_attrs) noexcept -> std::optional<error>;
void wait_idle() noexcept;
void clear(const config::clear_color &w_ccl) noexcept;

} // namespace surge::renderer::gl

#endif // SURGE_CORE_RENDERER_OPENGL_HPP