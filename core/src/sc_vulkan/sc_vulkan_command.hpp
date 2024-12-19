#ifndef SURGE_RENDERER_VULKAN_COMMAND_HPP
#define SURGE_RENDERER_VULKAN_COMMAND_HPP

#include "sc_integer_types.hpp"

// clang-format off
#include <vulkan/vulkan.h>
// clang-format on

namespace surge::renderer::vk {

auto command_pool_create_info(u32 queue_family_idx,
                              VkCommandPoolCreateFlags flags = 0) -> VkCommandPoolCreateInfo;

auto command_buffer_alloc_info(VkCommandPool pool, u32 count = 1) -> VkCommandBufferAllocateInfo;

auto command_buffer_begin_info(VkCommandBufferUsageFlags flags = 0) -> VkCommandBufferBeginInfo;

auto command_buffer_submit_info(VkCommandBuffer cmd) -> VkCommandBufferSubmitInfo;

auto submit_info(VkCommandBufferSubmitInfo *cmd, VkSemaphoreSubmitInfo *signam_sem_info,
                 VkSemaphoreSubmitInfo *wait_sem_info) -> VkSubmitInfo2;

} // namespace surge::renderer::vk

#endif // SURGE_RENDERER_VULKAN_COMMAND_HPP