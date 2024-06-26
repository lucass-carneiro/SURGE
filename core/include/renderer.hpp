#ifndef SURGE_CORE_RENDERER_HPP
#define SURGE_CORE_RENDERER_HPP

#include "config.hpp"
#include "error_types.hpp"

// clang-format off
#define GLFW_INCLUDE_VULKAN
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

struct context {
  VkInstance instance{nullptr};
};

auto init(const config::window_attrs &w_attrs) noexcept -> tl::expected<context, error>;
void terminate(context &ctx);

} // namespace vk

} // namespace surge::renderer

#endif // SURGE_CORE_RENDERER_HPP