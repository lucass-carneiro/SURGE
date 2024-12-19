#include "sc_config.hpp"
#include "sc_logging.hpp"
#include "sc_vulkan/sc_vulkan.hpp"
#include "sc_vulkan_init.hpp"
#include "sc_vulkan_malloc.hpp"
#include "sc_vulkan_types.hpp"

#include <vulkan/vk_enum_string_helper.h>

#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
#  include <tracy/Tracy.hpp>
#endif

auto surge::renderer::vk::request_img(context ctx) -> tl::expected<std::tuple<image, u32>, error> {

#if (defined(SURGE_BUILD_TYPE_Profile) || defined(SURGE_BUILD_TYPE_RelWithDebInfo))                \
    && defined(SURGE_ENABLE_TRACY)
  ZoneScopedN("SWPC Acquire");
#endif

  auto &dev{ctx->device};
  auto &swpc{ctx->swpc_data.swapchain};
  auto &render_fence{ctx->frm_data.render_fences[ctx->frm_data.frame_idx]};
  auto &swpc_semaphore{ctx->frm_data.swpc_semaphores[ctx->frm_data.frame_idx]};

  // Wait until the gpu has finished rendering the last frame. Timeout of 1 sec
  auto result{vkWaitForFences(dev, 1, &render_fence, true, 1000000000)};

  if (result != VK_SUCCESS) {
    log_error("Unable to wait render fence: {}", string_VkResult(result));
    return tl::unexpected{error::vk_surface_init};
  }

  result = vkResetFences(dev, 1, &render_fence);

  if (result != VK_SUCCESS) {
    log_error("Unable to reset render fence: {}", string_VkResult(result));
    return tl::unexpected{error::vk_surface_init};
  }

  // Request new image from the swapchain
  u32 swpc_img_idx{0};
  result = vkAcquireNextImageKHR(dev, swpc, 1000000000, swpc_semaphore, nullptr, &swpc_img_idx);

  if (result != VK_SUCCESS) {
    log_error("Unable to acquire swapchain image: {}", string_VkResult(result));
    return tl::unexpected{error::vk_get_swpc_img};
  }

  return std::make_tuple(ctx->swpc_data.imgs[swpc_img_idx], swpc_img_idx);
}

auto surge::renderer::vk::present(context ctx, u32 &swpc_img_idx,
                                  const config::renderer_attrs &r_attrs,
                                  const config::window_resolution &w_res) -> std::optional<error> {
  // Prepare present. This will put the image we just rendered to into the visible window. we want
  // to wait on the render_semaphore for that, as its necessary that drawing commands have finished
  // before the image is displayed to the user

  auto &graphics_queue{ctx->q_handles.graphics};
  auto &swpc{ctx->swpc_data.swapchain};
  auto &render_semaphore{ctx->frm_data.render_semaphores[ctx->frm_data.frame_idx]};

  VkPresentInfoKHR present_info{};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.pNext = nullptr;
  present_info.pSwapchains = &swpc;
  present_info.swapchainCount = 1;

  present_info.pWaitSemaphores = &render_semaphore;
  present_info.waitSemaphoreCount = 1;

  present_info.pImageIndices = &swpc_img_idx;

  const auto result{vkQueuePresentKHR(graphics_queue, &present_info)};

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    log_warn("Recreating swapchain");
    vkDeviceWaitIdle(ctx->device);

    const auto alloc_callbacks{get_alloc_callbacks()};

    for (const auto &img_view : ctx->swpc_data.imgs_views) {
      vkDestroyImageView(ctx->device, img_view, alloc_callbacks);
    }

    vkDestroySwapchainKHR(ctx->device, ctx->swpc_data.swapchain, alloc_callbacks);

    const auto swpc_data{create_swapchain(ctx->phys_dev, ctx->device, ctx->surface, r_attrs,
                                          static_cast<u32>(w_res.width),
                                          static_cast<u32>(w_res.height))};
    if (!swpc_data) {
      log_error("Unable to recreate swapchain");
      return error::vk_present;
    } else {
      ctx->swpc_data = *swpc_data;
    }
  } else if (result != VK_SUCCESS) {
    log_error("Unable to present rendering: {}", string_VkResult(result));
    return error::vk_present;
  }

  // increase the number of frames drawn
  ctx->frm_data.advance_idx();

  return {};
}