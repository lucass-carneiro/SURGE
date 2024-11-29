#ifndef SURGE_CORE_RENDERER_VK_SYNC_HPP
#define SURGE_CORE_RENDERER_VK_SYNC_HPP

// clang-format off
#include <vulkan/vulkan.h>
// clang-format on

namespace surge::renderer::vk {

auto fence_create_info(VkFenceCreateFlags flags = 0) noexcept -> VkFenceCreateInfo;

auto semaphore_create_info(VkSemaphoreCreateFlags flags = 0) noexcept -> VkSemaphoreCreateInfo;

auto semaphore_submit_info(VkPipelineStageFlags2 stage_mask,
                           VkSemaphore semaphore) noexcept -> VkSemaphoreSubmitInfo;

} // namespace surge::renderer::vk

#endif // SURGE_CORE_RENDERER_VK_SYNC_HPP