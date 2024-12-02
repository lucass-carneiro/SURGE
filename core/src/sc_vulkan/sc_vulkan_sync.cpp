#include "sc_vulkan/sc_vulkan_sync.hpp"

auto surge::renderer::vk::fence_create_info(VkFenceCreateFlags flags) noexcept
    -> VkFenceCreateInfo {
  VkFenceCreateInfo ci{};
  ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  ci.pNext = nullptr;
  ci.flags = flags;
  return ci;
}

auto surge::renderer::vk::semaphore_create_info(VkSemaphoreCreateFlags flags) noexcept
    -> VkSemaphoreCreateInfo {
  VkSemaphoreCreateInfo ci{};
  ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  ci.pNext = nullptr;
  ci.flags = flags;
  return ci;
}

auto surge::renderer::vk::semaphore_submit_info(VkPipelineStageFlags2 stage_mask,
                                                VkSemaphore semaphore) noexcept
    -> VkSemaphoreSubmitInfo {
  VkSemaphoreSubmitInfo si{};
  si.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
  si.pNext = nullptr;
  si.semaphore = semaphore;
  si.stageMask = stage_mask;
  si.deviceIndex = 0;
  si.value = 1;
  return si;
}