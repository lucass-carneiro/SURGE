#ifndef SURGE_CORE_RENDERER_HPP
#define SURGE_CORE_RENDERER_HPP

#include "config.hpp"
#include "error_types.hpp"
#include "options.hpp"
#include "vk_bootstrap/VkBootstrap.hpp"

// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
// clang-format on

#include <optional>
#include <tl/expected.hpp>

namespace surge::renderer {

auto init_opengl(const config::renderer_attrs &r_attrs) noexcept -> std::optional<error>;

namespace vk {

struct swapchain_data {
  vkb::Swapchain swapchain{};
  vector<VkImage> imgs{};
  vector<VkImageView> imgs_views{};
};

struct context {
  vkb::Instance instance{};
  VkSurfaceKHR surface{};
  vkb::PhysicalDevice phys_device{};
  vkb::Device device{};
  swapchain_data swpc_data{};
};

auto init(const config::renderer_attrs &r_attrs, const config::window_resolution &w_res,
          const config::window_attrs &w_attrs) noexcept -> tl::expected<context, error>;
void terminate(context &ctx);

auto create_swapchain(const config::renderer_attrs &r_attrs, context &ctx, u32 width,
                      u32 height) noexcept -> tl::expected<swapchain_data, error>;
void destroy_swapchain(context &ctx, swapchain_data &swpc) noexcept;

} // namespace vk

} // namespace surge::renderer

#endif // SURGE_CORE_RENDERER_HPP