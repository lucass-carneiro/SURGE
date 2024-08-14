#include "renderer_vk.hpp"

#include "logging.hpp"
#include "renderer_vk_command.hpp"
#include "renderer_vk_debug.hpp"
#include "renderer_vk_images.hpp"
#include "renderer_vk_init.hpp"
#include "renderer_vk_malloc.hpp"
#include "renderer_vk_sync.hpp"

#include <vulkan/vk_enum_string_helper.h>

auto surge::renderer::vk::init(
    const config::renderer_attrs &r_attrs, const config::window_resolution &w_res,
    const config::window_attrs &) noexcept -> tl::expected<context, error> {

  log_info("Initializing Vulkan");

  context ctx{};

  const auto instance{create_instance()};
  if (!instance) {
    return tl::unexpected{instance.error()};
  } else {
    ctx.instance = *instance;
  }

#ifdef SURGE_USE_VK_VALIDATION_LAYERS
  const auto dbg_msg{create_dbg_msg(*instance)};
  if (!dbg_msg) {
    return tl::unexpected{dbg_msg.error()};
  } else {
    ctx.dbg_msg = *dbg_msg;
  }
#endif

  const auto phys_dev{select_physical_device(*instance)};
  if (!phys_dev) {
    return tl::unexpected{phys_dev.error()};
  } else {
    ctx.phys_dev = *phys_dev;
  }

  const auto log_dev{create_logical_device(*phys_dev)};
  if (!log_dev) {
    return tl::unexpected{log_dev.error()};
  } else {
    ctx.log_dev = *log_dev;
  }

  const auto surface{create_window_surface(*instance)};
  if (!surface) {
    return tl::unexpected{surface.error()};
  } else {
    ctx.surface = *surface;
  }

  const auto q_handles{get_queue_handles(*phys_dev, *log_dev, *surface)};
  if (!q_handles) {
    return tl::unexpected{q_handles.error()};
  } else {
    ctx.q_handles = *q_handles;
  }

  const auto swpc_data{create_swapchain(*phys_dev, *log_dev, *surface, r_attrs,
                                        static_cast<u32>(w_res.width),
                                        static_cast<u32>(w_res.height))};
  if (!swpc_data) {
    return tl::unexpected{q_handles.error()};
  } else {
    ctx.swpc_data = *swpc_data;
  }

  const auto frm_data{create_frame_data(*log_dev, q_handles->graphics_idx)};
  if (!frm_data) {
    return tl::unexpected{q_handles.error()};
  } else {
    ctx.frm_data = *frm_data;
  }

  const auto allocator{create_memory_allocator(*instance, *phys_dev, *log_dev)};
  if (!allocator) {
    return tl::unexpected{q_handles.error()};
  } else {
    ctx.allocator = *allocator;
  }

  const auto draw_image{create_draw_img(w_res, *log_dev, *allocator)};
  if (!draw_image) {
    return tl::unexpected{q_handles.error()};
  } else {
    ctx.draw_image = *draw_image;
  }

  return ctx;
}

void surge::renderer::vk::terminate(context &ctx) noexcept {
  log_info("Terminating vulkan");

  log_info("Waiting for GPU idle");
  vkWaitForFences(ctx.log_dev, ctx.frm_data.frame_overlap, ctx.frm_data.render_fences.data(), true,
                  1000000000);

  log_info("Destroying draw image");
  vkDestroyImageView(ctx.log_dev, ctx.draw_image.image_view, get_alloc_callbacks());
  vmaDestroyImage(ctx.allocator, ctx.draw_image.image, ctx.draw_image.allocation);

  log_info("Destroying memory allocator");
  vmaDestroyAllocator(ctx.allocator);

  destroy_frame_data(ctx);

  log_info("Destroying image views");
  for (const auto &img_view : ctx.swpc_data.imgs_views) {
    vkDestroyImageView(ctx.log_dev, img_view, get_alloc_callbacks());
  }

  log_info("Destroying swapchain");
  vkDestroySwapchainKHR(ctx.log_dev, ctx.swpc_data.swapchain, get_alloc_callbacks());

  log_info("Destroying window surface");
  vkDestroySurfaceKHR(ctx.instance, ctx.surface, get_alloc_callbacks());

  log_info("Destroying logical device");
  vkDestroyDevice(ctx.log_dev, get_alloc_callbacks());

  destroy_dbg_msg(ctx.instance, ctx.dbg_msg);

  log_info("Destroying instance");
  vkDestroyInstance(ctx.instance, get_alloc_callbacks());
}

auto surge::renderer::vk::clear(context &ctx,
                                const config::clear_color &w_ccl) noexcept -> std::optional<error> {
  // Aliases
  auto &dev{ctx.log_dev};
  auto &graphics_queue{ctx.q_handles.graphics};
  auto &swpc{ctx.swpc_data.swapchain};
  auto &render_fence{ctx.frm_data.render_fences[ctx.frm_data.frame_idx]};
  auto &swpc_semaphore{ctx.frm_data.swpc_semaphores[ctx.frm_data.frame_idx]};
  auto &render_semaphore{ctx.frm_data.render_semaphores[ctx.frm_data.frame_idx]};
  auto &cmd_buff{ctx.frm_data.command_buffers[ctx.frm_data.frame_idx]};

  // Wait until the gpu has finished rendering the last frame. Timeout of 1 sec
  auto result{vkWaitForFences(dev, 1, &render_fence, true, 1000000000)};

  if (result != VK_SUCCESS) {
    log_error("Unable to wait render fence: {}", string_VkResult(result));
    return error::vk_surface_init;
  }

  result = vkResetFences(dev, 1, &render_fence);

  if (result != VK_SUCCESS) {
    log_error("Unable to reset render fence: {}", string_VkResult(result));
    return error::vk_surface_init;
  }

  // Request new image from the swapchain
  u32 swpc_img_idx{0};
  result = vkAcquireNextImageKHR(dev, swpc, 1000000000, swpc_semaphore, nullptr, &swpc_img_idx);

  if (result != VK_SUCCESS) {
    log_error("Unable to acquire swapchain image: {}", string_VkResult(result));
    return error::vk_get_swpc_img;
  }

  auto swpc_img{ctx.swpc_data.imgs[swpc_img_idx]};

  // Now that we are sure that the commands finished executing, we can safely reset the command
  // buffer to begin recording again.
  result = vkResetCommandBuffer(cmd_buff, 0);

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

  // transition our main draw image into general layout so we can write into it we will overwrite it
  // all so we dont care about what was the older layout
  transition_image(cmd_buff, ctx.draw_image.image, VK_IMAGE_LAYOUT_UNDEFINED,
                   VK_IMAGE_LAYOUT_GENERAL);

  // Clear color
  VkClearColorValue clear_value{{w_ccl.r, w_ccl.g, w_ccl.b, w_ccl.a}};

  VkImageSubresourceRange clear_range{image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT)};
  vkCmdClearColorImage(cmd_buff, ctx.draw_image.image, VK_IMAGE_LAYOUT_GENERAL, &clear_value, 1,
                       &clear_range);

  // Transition the draw image and the swapchain image into their correct transfer layouts
  transition_image(cmd_buff, ctx.draw_image.image, VK_IMAGE_LAYOUT_GENERAL,
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
  transition_image(cmd_buff, swpc_img, VK_IMAGE_LAYOUT_UNDEFINED,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  // Copy draw image to swapchain image
  VkExtent2D draw_image_ext_2d{ctx.draw_image.image_extent.width,
                               ctx.draw_image.image_extent.height};
  image_blit(cmd_buff, ctx.draw_image.image, swpc_img, draw_image_ext_2d, ctx.swpc_data.extent);

  // Make the swapchain image presentable
  transition_image(cmd_buff, swpc_img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  // Finalize the command buffer
  result = vkEndCommandBuffer(cmd_buff);

  if (result != VK_SUCCESS) {
    log_error("Unable to end command buffer recording: {}", string_VkResult(result));
    return error::vk_cmd_buff_rec_end;
  }

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
  result = vkQueueSubmit2(graphics_queue, 1, &sub_info, render_fence);

  if (result != VK_SUCCESS) {
    log_error("Unable to sumbit command buffer to graphics queue: {}", string_VkResult(result));
    return error::vk_cmd_buff_submit;
  }

  // Prepare present this will put the image we just rendered to into the visible window. we want to
  // wait on the _renderSemaphore for that,  as its necessary that drawing commands have finished
  // before the image is displayed to the user
  VkPresentInfoKHR present_info{};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.pNext = nullptr;
  present_info.pSwapchains = &swpc;
  present_info.swapchainCount = 1;

  present_info.pWaitSemaphores = &render_semaphore;
  present_info.waitSemaphoreCount = 1;

  present_info.pImageIndices = &swpc_img_idx;

  result = vkQueuePresentKHR(graphics_queue, &present_info);

  if (result != VK_SUCCESS) {
    log_error("Unable to present rendering: {}", string_VkResult(result));
    return error::vk_present;
  }

  // increase the number of frames drawn
  ctx.frm_data.advance_idx();

  return {};
}