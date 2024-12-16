#include "sc_vulkan_command.hpp"

#include "sc_logging.hpp"
#include "sc_vulkan/sc_vulkan.hpp"
#include "sc_vulkan_sync.hpp"

#include <vulkan/vk_enum_string_helper.h>

auto surge::renderer::vk::command_pool_create_info(u32 queue_family_idx,
                                                   VkCommandPoolCreateFlags flags) noexcept
    -> VkCommandPoolCreateInfo {
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

auto surge::renderer::vk::submit_info(VkCommandBufferSubmitInfo *cmd,
                                      VkSemaphoreSubmitInfo *signam_sem_info,
                                      VkSemaphoreSubmitInfo *wai_sem_info) noexcept
    -> VkSubmitInfo2 {
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

auto surge::renderer::vk::cmd_begin(context &ctx) noexcept -> std::optional<error> {
  auto &cmd_buff{ctx.frm_data.command_buffers[ctx.frm_data.frame_idx]};

  // Now that we are sure that the commands finished executing, we can safely reset the command
  // buffer to begin recording again.
  auto result{vkResetCommandBuffer(cmd_buff, 0)};

  if (result != VK_SUCCESS) {
    log_error("Unable to reset command buffer: {}", string_VkResult(result));
    return error::vk_cmd_buff_reset;
  }

  // Begin the command buffer recording. We will use this command buffer exactly once, so we want to
  // let vulkan know that
  auto cmd_beg_info{command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)};

  // Start the command buffer recording
  result = vkBeginCommandBuffer(cmd_buff, &cmd_beg_info);

  if (result != VK_SUCCESS) {
    log_error("Unable to start command buffer recording: {}", string_VkResult(result));
    return error::vk_cmd_buff_rec_start;
  }

  return {};
}

auto surge::renderer::vk::cmd_end(context &ctx) noexcept -> std::optional<error> {
  auto &cmd_buff{ctx.frm_data.command_buffers[ctx.frm_data.frame_idx]};

  // Finalize the command buffer
  const auto result{vkEndCommandBuffer(cmd_buff)};

  if (result != VK_SUCCESS) {
    log_error("Unable to end command buffer recording: {}", string_VkResult(result));
    return error::vk_cmd_buff_rec_end;
  }

  return {};
}

auto surge::renderer::vk::cmd_submit(context &ctx) noexcept -> std::optional<error> {
  auto &graphics_queue{ctx.q_handles.graphics};
  auto &render_fence{ctx.frm_data.render_fences[ctx.frm_data.frame_idx]};
  auto &swpc_semaphore{ctx.frm_data.swpc_semaphores[ctx.frm_data.frame_idx]};
  auto &render_semaphore{ctx.frm_data.render_semaphores[ctx.frm_data.frame_idx]};
  auto &cmd_buff{ctx.frm_data.command_buffers[ctx.frm_data.frame_idx]};

  // Prepare the submission to the queue. we want to wait on the _presentSemaphore, as that
  // semaphore is signaled when the swapchain is ready we will signal the _renderSemaphore, to
  // signal that rendering has finished.
  auto cmd_sub_info{command_buffer_submit_info(cmd_buff)};

  auto signal_info{semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, render_semaphore)};
  auto wait_info{
      semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, swpc_semaphore)};

  auto sub_info{submit_info(&cmd_sub_info, &signal_info, &wait_info)};

  // Submit command buffer to the queue and execute it. The render fence will now block until the
  // graphic commands finish execution
  const auto result{vkQueueSubmit2(graphics_queue, 1, &sub_info, render_fence)};

  if (result != VK_SUCCESS) {
    log_error("Unable to sumbit command buffer to graphics queue: {}", string_VkResult(result));
    return error::vk_cmd_buff_submit;
  }

  return {};
}
