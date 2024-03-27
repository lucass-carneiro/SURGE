#ifndef SURGE_RENDERER_HPP
#define SURGE_RENDERER_HPP

#include "VkBootstrap.hpp"
#include "container_types.hpp"
#include "error_types.hpp"

#include <tl/expected.hpp>

namespace surge::renderer {

struct context {
  vkb::Instance instance{};
  vkb::Device device{};

  VkSurfaceKHR surface{};

  vkb::Swapchain swapchain{};
};

auto init(const string &window_name, GLFWwindow *window) noexcept -> tl::expected<context, error>;
void terminate(context &ctx) noexcept;

} // namespace surge::renderer

#endif // SURGE_RENDERER_HPP