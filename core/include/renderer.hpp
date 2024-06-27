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

struct frame_cmd_data {
  VkCommandPool pool{};
  VkCommandBuffer buffer{};
};

struct command_data {
  static constexpr usize frame_overlap{2};

  usize frame_number{0};

  std::array<VkCommandPool, frame_overlap> command_pools{};
  std::array<VkCommandBuffer, frame_overlap> command_buffers{};

  u32 graphics_queue_family{0};
  VkQueue graphics_queue{nullptr};

  [[nodiscard]] auto get_frame_cmd_data() noexcept -> frame_cmd_data;
};

struct context {
  vkb::Instance instance{};
  VkSurfaceKHR surface{};

  vkb::PhysicalDevice phys_device{};
  vkb::Device device{};

  swapchain_data swpc_data{};
  command_data cmd_data{};
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