#ifndef SURGE_CORE_RENDERER_VULKAN_HPP
#define SURGE_CORE_RENDERER_VULKAN_HPP

#include "sc_config.hpp"
#include "sc_vulkan_types.hpp"

#include <tl/expected.hpp>

namespace surge::renderer::vk {

auto initialize(const config::renderer_attrs &r_attrs, const config::window_resolution &w_res,
                const config::window_attrs &w_attrs) noexcept -> tl::expected<context, error>;

void terminate(context &ctx) noexcept;

auto request_img(context &ctx) noexcept -> tl::expected<std::tuple<VkImage, u32>, error>;
auto present(context &ctx, u32 &swpc_img_idx) noexcept -> std::optional<error>;

auto cmd_begin(context &ctx) noexcept -> std::optional<error>;
auto cmd_end(context &ctx) noexcept -> std::optional<error>;
auto cmd_submit(context &ctx) noexcept -> std::optional<error>;

void clear(context &ctx, VkImage swpc_image, const config::clear_color &w_ccl) noexcept;

} // namespace surge::renderer::vk

#endif // SURGE_CORE_RENDERER_VULKAN_HPP