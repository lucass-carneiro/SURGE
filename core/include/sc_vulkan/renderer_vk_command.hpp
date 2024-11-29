#ifndef SURGE_RENDERER_VK_COMMAND_HPP
#define SURGE_RENDERER_VK_COMMAND_HPP

#include "integer_types.hpp"

// clang-format off
#include <vulkan/vulkan.h>
// clang-format on

namespace surge::renderer::vk {

auto command_pool_create_info(u32 queue_family_idx, VkCommandPoolCreateFlags flags
                                                    = 0) noexcept -> VkCommandPoolCreateInfo;
auto command_buffer_alloc_info(VkCommandPool pool,
                               u32 count = 1) noexcept -> VkCommandBufferAllocateInfo;

auto command_buffer_begin_info(VkCommandBufferUsageFlags flags
                               = 0) noexcept -> VkCommandBufferBeginInfo;

auto command_buffer_submit_info(VkCommandBuffer cmd) noexcept -> VkCommandBufferSubmitInfo;

auto submit_info(VkCommandBufferSubmitInfo *cmd, VkSemaphoreSubmitInfo *signam_sem_info,
                 VkSemaphoreSubmitInfo *wait_sem_info) noexcept -> VkSubmitInfo2;

} // namespace surge::renderer::vk

#endif // SURGE_RENDERER_VK_COMMAND_HPP