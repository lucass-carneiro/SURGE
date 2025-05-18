#ifndef SURGE_RENDERER_VULKAN_MALLOC_HPP
#define SURGE_RENDERER_VULKAN_MALLOC_HPP

#include "sc_error_types.hpp"
#include "sc_vulkan_types.hpp"

// clang-format off
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
// clang-format on

namespace surge::renderer::vk {

auto get_alloc_callbacks() -> const VkAllocationCallbacks *;

auto create_memory_allocator(VkInstance instance, VkPhysicalDevice phys_dev,
                             VkDevice logi_dev) -> Result<VmaAllocator>;

auto create_buffer(Context ctx, size_t size, VkBufferUsageFlags usage_flags,
                   VmaMemoryUsage memory_usage) -> Result<Buffer>;

void destroy_buffer(Context ctx, Buffer buff);

} // namespace surge::renderer::vk

#endif // SURGE_RENDERER_VULKAN_MALLOC_HPP