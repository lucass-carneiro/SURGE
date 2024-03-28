#include "renderer.hpp"

#include "allocators.hpp"
#include "error_types.hpp"
#include "integer_types.hpp"
#include "logging.hpp"
#include "options.hpp"

static auto vk_alloc(void *, surge::usize size, surge::usize alignment,
                     [[maybe_unused]] VkSystemAllocationScope scope) noexcept -> void * {
  auto p{surge::allocators::mimalloc::malloc(size, alignment)};
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Vulkan Memory Event\n"
            "---\n"
            "type: alloc\n"
            "allocator: \"mimalloc::malloc\"\n"
            "size: %zu\n"
            "address: %p\n"
            "failed: %s"
            "scope: %i",
            size, p, p ? "false" : "true", scope);
#endif
  return p;
}

static auto vk_realloc(void *, void *original, surge::usize size, surge::usize alignment,
                       [[maybe_unused]] VkSystemAllocationScope scope) noexcept -> void * {
  auto q{surge::allocators::mimalloc::realloc(original, size, alignment)};
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Vulkan Memory Event\n"
            "---\n"
            "type: realloc\n"
            "allocator: \"mimalloc::realloc\"\n"
            "new size: %zu\n"
            "old address: %p\n"
            "new address: %p\n"
            "failed: %s"
            "scope: %i",
            size, original, q, q ? "false" : "true", scope);
#endif
  return q;
}

static void vk_free(void *, void *p) noexcept {
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Vulkan Memory Event\n"
            "---\n"
            "type: free\n"
            "allocator: \"mimalloc::free\"\n"
            "address: %p",
            p);
#endif
  surge::allocators::mimalloc::free(p);
}

static void vk_internal_alloc(void *, [[maybe_unused]] surge::usize size,
                              [[maybe_unused]] VkInternalAllocationType type,
                              [[maybe_unused]] VkSystemAllocationScope scope) noexcept {
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Vulkan Internal Memory Event\n"
            "---\n"
            "type: alloc\n"
            "allocator: \"mimalloc::malloc\"\n"
            "size: %zu\n"
            "type: %i\n"
            "scope: %i",
            size, type, scope);
#endif
}

static void vk_internal_free(void *, [[maybe_unused]] surge::usize size,
                             [[maybe_unused]] VkInternalAllocationType type,
                             [[maybe_unused]] VkSystemAllocationScope scope) noexcept {
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Vulkan Internal Memory Event\n"
            "---\n"
            "type: free\n"
            "allocator: \"mimalloc::malloc\"\n"
            "size: %zu\n"
            "type: %i\n"
            "scope: %i",
            size, type, scope);
#endif
}

static VKAPI_ATTR auto VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data, void *) noexcept -> VkBool32 {

  switch (severity) {
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
    log_info("Vulkan Diagnostic: %s", callback_data->pMessage);
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
    log_info("Vulkan Info: %s", callback_data->pMessage);
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
    log_warn("Vulkan Warning: %s", callback_data->pMessage);
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
    log_error("Vulkan Error: %s", callback_data->pMessage);
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
    break;
  }

  return VK_FALSE;
}

// NOLINTNEXTLINE
static VkAllocationCallbacks vk_alloc_callbacks{nullptr, vk_alloc,          vk_realloc,
                                                vk_free, vk_internal_alloc, vk_internal_free};

static auto fence_create_info(VkFenceCreateFlags flags) noexcept -> VkFenceCreateInfo {
  VkFenceCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  info.pNext = nullptr;
  info.flags = flags;
  return info;
}

static auto semaphore_create_info(VkSemaphoreCreateFlags flags) noexcept -> VkSemaphoreCreateInfo {
  VkSemaphoreCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  info.pNext = nullptr;
  info.flags = flags;
  return info;
}

auto surge::renderer::init(const config::window_attrs &wattrs, GLFWwindow *window) noexcept
    -> tl::expected<context, error> {
  log_info("Initializing Vulkan");

  context ctx{};

  /************
   * Instance *
   ************/
  vkb::InstanceBuilder ib{};

#ifdef SURGE_BUILD_TYPE_Debug
  ib.request_validation_layers(true);
#else
  ib.request_validation_layers(false);
#endif

  ib.set_debug_callback(debug_callback);
  ib.set_allocation_callbacks(&vk_alloc_callbacks);
  ib.set_app_name(wattrs.name.c_str());
  ib.set_engine_name("SURGE - The Super Underrated Game Engine");
  ib.require_api_version(1, 3, 0);

  auto ib_result{ib.build()};
  if (!ib_result) {
    log_error("Unable to create Vulkan instance: %s", ib_result.error().message().c_str());
    return tl::unexpected{error::vk_instance_creation};
  }

  /***********
   * Surface *
   ***********/
  VkSurfaceKHR surface{};
  if (glfwCreateWindowSurface(ib_result->instance, window, &vk_alloc_callbacks, &surface)
      != VK_SUCCESS) {
    log_error("Unable to acquire Vulkan surface");
    return tl::unexpected{error::vk_surface};
  }

  /************
   * Device *
   ************/
  // vulkan 1.3 features
  VkPhysicalDeviceVulkan13Features features{};
  features.dynamicRendering = true;
  features.synchronization2 = true;

  // vulkan 1.2 features
  VkPhysicalDeviceVulkan12Features features12{};
  features12.bufferDeviceAddress = true;
  features12.descriptorIndexing = true;

  vkb::PhysicalDeviceSelector pds{ib_result.value()};
  pds.prefer_gpu_device_type(vkb::PreferredDeviceType::discrete);
  pds.set_minimum_version(1, 3);
  pds.set_required_features_13(features);
  pds.set_required_features_12(features12);
  pds.set_surface(surface);

  auto pds_result{pds.select()};

  if (!pds_result) {
    log_error("Unable to select physical device: %s", pds_result.error().message().c_str());
    return tl::unexpected{error::vk_physical_device};
  }

  vkb::DeviceBuilder db{pds_result.value()};
  db.set_allocation_callbacks(&vk_alloc_callbacks);

  auto db_result{db.build()};
  if (!db_result) {
    log_error("Unable to build logical device: %s", db_result.error().message().c_str());
    return tl::unexpected{error::vk_logical_device};
  }

  /*************
   * Swapchain *
   *************/
  const auto sc_img_format{VK_FORMAT_B8G8R8A8_UNORM};
  int ww{0}, wh{0};
  glfwGetWindowSize(window, &ww, &wh);

  vkb::SwapchainBuilder scb{pds_result.value().physical_device, db_result.value().device, surface};
  scb.set_desired_format(
      VkSurfaceFormatKHR{.format = sc_img_format, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR});

  if (wattrs.vsync) {
    scb.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR);
  } else {
    scb.set_desired_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR);
  }

  scb.set_desired_extent(static_cast<u32>(ww), static_cast<u32>(wh));
  scb.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT);
  scb.set_allocation_callbacks(&vk_alloc_callbacks);

  auto scb_result{scb.build()};
  if (!scb_result) {
    log_error("Unable to build swapchain: %s", scb_result.error().message().c_str());
    return tl::unexpected{error::vk_swapchain_error};
  }

  auto sc_imgs_result{scb_result.value().get_images()};
  if (!sc_imgs_result) {
    log_error("Unable to retrieve swapchain images: %s", sc_imgs_result.error().message().c_str());
    return tl::unexpected{error::vk_swapchain_images};
  }

  auto sc_img_views_result{scb_result.value().get_image_views()};
  if (!sc_img_views_result) {
    log_error("Unable to retrieve swapchain image views: %s",
              sc_img_views_result.error().message().c_str());
    return tl::unexpected{error::vk_swapchain_image_views};
  }

  /**********
   * Queues *
   **********/

  auto get_gqueue_result{db_result.value().get_queue(vkb::QueueType::graphics)};
  if (!get_gqueue_result) {
    log_error("Unable to obtain graphics queue from the device: %s",
              get_gqueue_result.error().message().c_str());
    return tl::unexpected{error::vk_graphics_queue};
  }

  auto get_gqueue_index_result{db_result.value().get_queue_index(vkb::QueueType::graphics)};
  if (!get_gqueue_index_result) {
    log_error("Unable to obtain graphics queue from the device: %s",
              get_gqueue_index_result.error().message().c_str());
    return tl::unexpected{error::vk_graphics_queue_index};
  }

  /**********************
   * Command structures *
   **********************/
  VkCommandPoolCreateInfo cpci = {};
  cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cpci.pNext = nullptr;
  cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  cpci.queueFamilyIndex = get_gqueue_index_result.value();

  for (usize i = 0; i < ctx.ofd.size(); i++) {
    if (vkCreateCommandPool(db_result->device, &cpci, &vk_alloc_callbacks, &ctx.ofd[i].pool)
        != VK_SUCCESS) {
      log_error("Unable to create Vulkan command pool");
      return tl::unexpected{error::vk_cmd_pool};
    }

    VkCommandBufferAllocateInfo cmd_alloc_info = {};
    cmd_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_alloc_info.pNext = nullptr;
    cmd_alloc_info.commandPool = ctx.ofd[i].pool;
    cmd_alloc_info.commandBufferCount = 1;
    cmd_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    if (vkAllocateCommandBuffers(db_result->device, &cmd_alloc_info, &ctx.ofd[i].buffer)
        != VK_SUCCESS) {
      log_error("Unable to create Vulkan command buffer");
      return tl::unexpected{error::vk_cmd_buffer};
    }
  }

  /*******************
   * Sync structures *
   *******************/
  auto fci{fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT)};
  auto sci{semaphore_create_info(0)};

  for (usize i = 0; i < ctx.ofd.size(); i++) {
    if (vkCreateFence(db_result->device, &fci, &vk_alloc_callbacks, &ctx.ofd[i].render_fence)
        != VK_SUCCESS) {
      log_error("Unable to initialize render fence");
      return tl::unexpected{error::vk_render_fence};
    }

    if (vkCreateSemaphore(db_result->device, &sci, &vk_alloc_callbacks, &ctx.ofd[i].sc_sem)
        != VK_SUCCESS) {
      log_error("Unable to create swap chain semaphore");
      return tl::unexpected{error::vk_sc_sem};
    }

    if (vkCreateSemaphore(db_result->device, &sci, &vk_alloc_callbacks, &ctx.ofd[i].render_sem)
        != VK_SUCCESS) {
      log_error("Unable to create render semaphore");
      return tl::unexpected{error::vk_sc_sem};
    }
  }

  log_info("Vulkan initialized");

  ctx.instance = ib_result.value();
  ctx.device = db_result.value();
  ctx.surface = surface;
  ctx.swapchain = scb_result.value();
  ctx.images = std::move(sc_imgs_result.value());
  ctx.image_views = std::move(sc_img_views_result.value());
  ctx.graphics_queue_family = get_gqueue_index_result.value();
  ctx.graphics_queue = get_gqueue_result.value();

  return ctx;
}

void surge::renderer::terminate(context &ctx) noexcept {
  log_info("Terminating Vulkan");

  vkDeviceWaitIdle(ctx.device.device);

  for (usize i = 0; i < ctx.ofd.size(); i++) {
    vkDestroySemaphore(ctx.device.device, ctx.ofd[i].render_sem, &vk_alloc_callbacks);
    vkDestroySemaphore(ctx.device.device, ctx.ofd[i].sc_sem, &vk_alloc_callbacks);
    vkDestroyFence(ctx.device.device, ctx.ofd[i].render_fence, &vk_alloc_callbacks);
    vkDestroyCommandPool(ctx.device.device, ctx.ofd[i].pool, &vk_alloc_callbacks);
  }

  for (auto &img : ctx.image_views) {
    vkDestroyImageView(ctx.device.device, img, &vk_alloc_callbacks);
  }

  vkb::destroy_swapchain(ctx.swapchain);
  vkDestroySurfaceKHR(ctx.instance.instance, ctx.surface, &vk_alloc_callbacks);
  vkb::destroy_device(ctx.device);
  vkb::destroy_instance(ctx.instance);
}

auto surge::renderer::context::get_current_frame() noexcept -> frame_data & {
  static usize idx{0};
  idx = idx % ofd.size();
  return ofd[idx];
}

auto surge::renderer::cmd_buff_beg_info(VkCommandBufferUsageFlags flags) noexcept
    -> VkCommandBufferBeginInfo {
  VkCommandBufferBeginInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  info.pNext = nullptr;
  info.pInheritanceInfo = nullptr;
  info.flags = flags;
  return info;
}

auto surge::renderer::image_subresource_range(VkImageAspectFlags aspectMask)
    -> VkImageSubresourceRange {
  VkImageSubresourceRange subImage{};
  subImage.aspectMask = aspectMask;
  subImage.baseMipLevel = 0;
  subImage.levelCount = VK_REMAINING_MIP_LEVELS;
  subImage.baseArrayLayer = 0;
  subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;

  return subImage;
}

void surge::renderer::transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout src,
                                       VkImageLayout dest) noexcept {

  VkImageMemoryBarrier2 img_barrier{.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
  img_barrier.pNext = nullptr;
  img_barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
  img_barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
  img_barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
  img_barrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
  img_barrier.oldLayout = src;
  img_barrier.newLayout = dest;

  VkImageAspectFlags aspect_mask{(dest == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
                                     ? VK_IMAGE_ASPECT_DEPTH_BIT
                                     : VK_IMAGE_ASPECT_COLOR_BIT};

  img_barrier.subresourceRange = image_subresource_range(aspect_mask);
  img_barrier.image = image;

  VkDependencyInfo depInfo{};
  depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  depInfo.pNext = nullptr;

  depInfo.imageMemoryBarrierCount = 1;
  depInfo.pImageMemoryBarriers = &img_barrier;

  vkCmdPipelineBarrier2(cmd, &depInfo);
}

auto surge::renderer::semaphore_submit_info(VkPipelineStageFlags2 stageMask,
                                            VkSemaphore semaphore) noexcept
    -> VkSemaphoreSubmitInfo {

  VkSemaphoreSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
  submitInfo.pNext = nullptr;
  submitInfo.semaphore = semaphore;
  submitInfo.stageMask = stageMask;
  submitInfo.deviceIndex = 0;
  submitInfo.value = 1;

  return submitInfo;
}

auto surge::renderer::command_buffer_submit_info(VkCommandBuffer cmd) noexcept
    -> VkCommandBufferSubmitInfo {

  VkCommandBufferSubmitInfo info{};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
  info.pNext = nullptr;
  info.commandBuffer = cmd;
  info.deviceMask = 0;

  return info;
}

auto surge::renderer::submit_info(VkCommandBufferSubmitInfo *cmd,
                                  VkSemaphoreSubmitInfo *signalSemaphoreInfo,
                                  VkSemaphoreSubmitInfo *waitSemaphoreInfo) noexcept
    -> VkSubmitInfo2 {

  VkSubmitInfo2 info{};

  info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
  info.pNext = nullptr;

  info.waitSemaphoreInfoCount = waitSemaphoreInfo == nullptr ? 0 : 1;
  info.pWaitSemaphoreInfos = waitSemaphoreInfo;

  info.signalSemaphoreInfoCount = signalSemaphoreInfo == nullptr ? 0 : 1;
  info.pSignalSemaphoreInfos = signalSemaphoreInfo;

  info.commandBufferInfoCount = 1;
  info.pCommandBufferInfos = cmd;

  return info;
}
