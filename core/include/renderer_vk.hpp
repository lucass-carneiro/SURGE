#ifndef SURGE_CORE_RENDERER_VK_HPP
#define SURGE_CORE_RENDERER_VK_HPP

#include "config.hpp"
#include "renderer_vk_types.hpp"

namespace surge::renderer::vk {

auto initialize(const config::renderer_attrs &r_attrs, const config::window_resolution &w_res,
                const config::window_attrs &w_attrs) noexcept -> tl::expected<context, error>;

void terminate(context &ctx) noexcept;

auto clear(context &ctx, const config::clear_color &w_ccl) noexcept -> std::optional<error>;

} // namespace surge::renderer::vk

#endif // SURGE_CORE_RENDERER_VK_HPP