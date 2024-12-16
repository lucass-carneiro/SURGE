#ifndef SURGE_RENDERER_VULKAN_MALLOC_HPP
#define SURGE_RENDERER_VULKAN_MALLOC_HPP

#include "sc_error_types.hpp"

// clang-format off
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
// clang-format on

#include <tl/expected.hpp>

namespace surge::renderer::vk {

auto get_alloc_callbacks() noexcept -> const VkAllocationCallbacks *;

auto create_memory_allocator(VkInstance instance, VkPhysicalDevice phys_dev,
                             VkDevice logi_dev) noexcept -> tl::expected<VmaAllocator, error>;

} // namespace surge::renderer::vk

#endif // SURGE_RENDERER_VULKAN_MALLOC_HPP