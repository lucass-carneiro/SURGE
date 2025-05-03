#ifndef SURGE_RENDERER_VULKAN_MALLOC_HPP
#define SURGE_RENDERER_VULKAN_MALLOC_HPP

#include "sc_error_types.hpp"

// clang-format off
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
// clang-format on

namespace surge::renderer::vk {

auto get_alloc_callbacks() -> const VkAllocationCallbacks *;

auto create_memory_allocator(VkInstance instance, VkPhysicalDevice phys_dev,
                             VkDevice logi_dev) -> Result<VmaAllocator>;

} // namespace surge::renderer::vk

#endif // SURGE_RENDERER_VULKAN_MALLOC_HPP