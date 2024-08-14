#include "renderer_vk_command.hpp"

auto surge::renderer::vk::command_pool_create_info(
    u32 queue_family_idx, VkCommandPoolCreateFlags flags) noexcept -> VkCommandPoolCreateInfo {
  VkCommandPoolCreateInfo ci{.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                             .pNext = nullptr,
                             .flags = flags,
                             .queueFamilyIndex = queue_family_idx};
  return ci;
}

auto surge::renderer::vk::command_buffer_alloc_info(VkCommandPool pool, u32 count) noexcept
    -> VkCommandBufferAllocateInfo {
  VkCommandBufferAllocateInfo ai{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                 .pNext = nullptr,
                                 .commandPool = pool,
                                 .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                 .commandBufferCount = count};
  return ai;
}

auto surge::renderer::vk::command_buffer_begin_info(VkCommandBufferUsageFlags flags) noexcept
    -> VkCommandBufferBeginInfo {
  VkCommandBufferBeginInfo ci = {};
  ci.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  ci.pNext = nullptr;
  ci.pInheritanceInfo = nullptr;
  ci.flags = flags;
  return ci;
}

auto surge::renderer::vk::command_buffer_submit_info(VkCommandBuffer cmd) noexcept
    -> VkCommandBufferSubmitInfo {
  VkCommandBufferSubmitInfo si{};
  si.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
  si.pNext = nullptr;
  si.commandBuffer = cmd;
  si.deviceMask = 0;
  return si;
}

auto surge::renderer::vk::submit_info(
    VkCommandBufferSubmitInfo *cmd, VkSemaphoreSubmitInfo *signam_sem_info,
    VkSemaphoreSubmitInfo *wai_sem_info) noexcept -> VkSubmitInfo2 {
  VkSubmitInfo2 si{};
  si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
  si.pNext = nullptr;

  si.waitSemaphoreInfoCount = wai_sem_info == nullptr ? 0 : 1;
  si.pWaitSemaphoreInfos = wai_sem_info;

  si.signalSemaphoreInfoCount = signam_sem_info == nullptr ? 0 : 1;
  si.pSignalSemaphoreInfos = signam_sem_info;

  si.commandBufferInfoCount = 1;
  si.pCommandBufferInfos = cmd;

  return si;
}