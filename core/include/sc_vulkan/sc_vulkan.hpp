#ifndef SURGE_CORE_RENDERER_VULKAN_NEW_HPP
#define SURGE_CORE_RENDERER_VULKAN_NEW_HPP

#include "sc_config.hpp"
#include "sc_glfw_includes.hpp"
#include "sc_integer_types.hpp"

#include <optional>
#include <tl/expected.hpp>

namespace surge::renderer::vk {

struct context_t;
using context = context_t *;

auto initialize(window::window_t w, const config::renderer_attrs &r_attrs,
                const config::window_resolution &w_res, const config::window_attrs &w_attrs)
    -> tl::expected<context, error>;

void terminate(context ctx);

auto request_swpc_img(context ctx) -> tl::expected<void, error>;
auto present_swpc(context ctx, const config::renderer_attrs &r_attrs,
                  const config::window_resolution &w_res) -> std::optional<error>;

auto cmd_begin(context ctx) -> std::optional<error>;
auto cmd_end(context ctx) -> std::optional<error>;
auto cmd_submit(context ctx) -> std::optional<error>;

void clear_swpc(context ctx, const config::clear_color &w_ccl);

} // namespace surge::renderer::vk

#endif // SURGE_CORE_RENDERER_VULKAN_NEW_HPP