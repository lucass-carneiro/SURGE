#include "allocators.hpp"
#include "cli.hpp"
#include "config.hpp"
#include "renderer.hpp"
#include "timers.hpp"
#include "window.hpp"

#include <cstdlib>

auto main(int, char **) noexcept -> int {
  using namespace surge;

  /*******************
   * Init allocators *
   *******************/
  allocators::mimalloc::init();

  /********
   * Logo *
   ********/
  cli::draw_logo();

  /*********************
   * Parse config file *
   *********************/
  const auto config_data{config::parse_config()};
  if (!config_data) {
    return EXIT_FAILURE;
  }

  const auto [w_res, w_ccl, w_attrs, first_mod] = *config_data;

  /***************
   * Init window *
   ***************/
  auto window{window::init(w_res, w_attrs)};
  if (!window) {
    return EXIT_FAILURE;
  }

  /*****************
   * Init Renderer *
   *****************/
  auto vk_ctx{renderer::init(w_attrs, *window)};
  if (!vk_ctx) {
    window::terminate(*window);
    return EXIT_FAILURE;
  }

  /***********************
   * Main Loop variables *
   ***********************/
  const u64 wait_tout{1000000000}; // In ns
  const VkClearColorValue vk_w_ccl{{w_ccl.r, w_ccl.g, w_ccl.b, w_ccl.a}};
  timers::generic_timer frame_timer;

  /*************
   * Main Loop *
   *************/
  while ((frame_timer.start(), !glfwWindowShouldClose(*window))) {
    /*
     * Events
     */
    glfwPollEvents();

    /*
     * Update
     */

    /*
     * Draw
     */
    // Wait until the gpu has finished rendering the last frame.
    vkWaitForFences(vk_ctx->device, 1, &vk_ctx->get_current_frame().render_fence, true, wait_tout);
    vkResetFences(vk_ctx->device, 1, &vk_ctx->get_current_frame().render_fence);

    // Get Swapchain image
    u32 scii{0};
    vkAcquireNextImageKHR(vk_ctx->device, vk_ctx->swapchain, wait_tout,
                          vk_ctx->get_current_frame().sc_sem, nullptr, &scii);

    // Reset command buffer
    auto cmd{vk_ctx->get_current_frame().buffer};
    vkResetCommandBuffer(cmd, 0);

    // Start the command buffer recording
    auto cbbi{renderer::cmd_buff_beg_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)};
    vkBeginCommandBuffer(cmd, &cbbi);

    // Make swapchain image writeable
    renderer::transition_image(cmd, vk_ctx->images[scii], VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_GENERAL);

    auto clear_range{renderer::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT)};

    // clear image
    vkCmdClearColorImage(cmd, vk_ctx->images[scii], VK_IMAGE_LAYOUT_GENERAL, &vk_w_ccl, 1,
                         &clear_range);

    // TODO: Module drawing code probably goes here

    // Make swapchain image presentable
    renderer::transition_image(cmd, vk_ctx->images[scii], VK_IMAGE_LAYOUT_GENERAL,
                               VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    // Stop command buffer recording
    vkEndCommandBuffer(cmd);

    // Prepare submission
    auto cmd_submit_info{renderer::command_buffer_submit_info(cmd)};

    auto sc_sem_info{renderer::semaphore_submit_info(
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, vk_ctx->get_current_frame().sc_sem)};

    auto render_sem_info{renderer::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                                                         vk_ctx->get_current_frame().render_sem)};

    auto submit_info{renderer::submit_info(&cmd_submit_info, &render_sem_info, &sc_sem_info)};

    // Submit and execute command buffer
    vkQueueSubmit2(vk_ctx->graphics_queue, 1, &submit_info,
                   vk_ctx->get_current_frame().render_fence);

    // Present rendering
    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;
    present_info.pSwapchains = &vk_ctx->swapchain.swapchain;
    present_info.swapchainCount = 1;
    present_info.pWaitSemaphores = &vk_ctx->get_current_frame().render_sem;
    present_info.waitSemaphoreCount = 1;
    present_info.pImageIndices = &scii;

    vkQueuePresentKHR(vk_ctx->graphics_queue, &present_info);

    frame_timer.stop();
  }

  // Normal shutdown
  renderer::terminate(*vk_ctx);
  window::terminate(*window);
  return EXIT_SUCCESS;
}