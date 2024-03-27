#ifndef SURGE_RENDERER_HPP
#define SURGE_RENDERER_HPP

#include "container_types.hpp"
#include "error_types.hpp"
#include "vulkan_headers.hpp"

#include <tl/expected.hpp>

namespace surge::renderer {

struct context {
  VkInstance instance{};
  VkAllocationCallbacks alloc_callbacks{};
  VkDebugUtilsMessengerEXT debug_messager{};
};

auto init(const string &window_name) noexcept -> tl::expected<context, error>;
void terminate(context &ctx) noexcept;

} // namespace surge::renderer

#endif // SURGE_RENDERER_HPP