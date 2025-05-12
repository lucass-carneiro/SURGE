#ifndef SURGE_CORE_RENDERER_VULKAN_HPP
#define SURGE_CORE_RENDERER_VULKAN_HPP

#include "sc_config.hpp"
#include "sc_error_types.hpp"
#include "sc_vulkan_types.hpp"
#include "sc_window.hpp"

namespace surge::renderer::vk {

auto initialize(window::Window w, const config::RendererAttributes &r_attrs,
                const config::WindowResolution &w_res) -> Result<Context>;

void terminate(Context ctx);

auto request_swpc_img(Context ctx) -> Result<void>;
auto present_swpc(Context ctx) -> Result<void>;
auto rebuild_swpc(Context ctx, const config::RendererAttributes &r_attrs,
                  const config::WindowResolution &w_res) -> Result<void>;

auto cmd_begin(Context ctx) -> Result<void>;
auto cmd_end(Context ctx) -> Result<void>;
auto cmd_submit(Context ctx) -> Result<void>;

void clear_swpc(Context ctx, const config::ClearColor &w_ccl);

} // namespace surge::renderer::vk

#endif // SURGE_CORE_RENDERER_VULKAN_HPP