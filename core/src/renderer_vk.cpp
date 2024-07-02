#include "allocators.hpp"
#include "logging.hpp"
#include "renderer.hpp"
#include "window.hpp"

#include <array>
#include <cstring>

static auto vk_malloc(void *, size_t size, size_t alignment,
                      [[maybe_unused]] VkSystemAllocationScope scope) noexcept -> void * {
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Vk memory event with scope {}", static_cast<int>(scope));
#endif
  return surge::allocators::mimalloc::aligned_alloc(size, alignment);
}

static auto vk_realloc(void *, void *pOriginal, size_t size, size_t alignment,
                       [[maybe_unused]] VkSystemAllocationScope scope) noexcept -> void * {
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Vk memory event with scope {}", static_cast<int>(scope));
#endif
  return surge::allocators::mimalloc::aligned_realloc(pOriginal, size, alignment);
}

static void vk_free(void *, void *pMemory) noexcept {
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Vk memory free event");
#endif
  surge::allocators::mimalloc::free(pMemory);
}

static void vk_internal_malloc([[maybe_unused]] void *, [[maybe_unused]] size_t size,
                               [[maybe_unused]] VkInternalAllocationType type,
                               [[maybe_unused]] VkSystemAllocationScope scope) {
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Vk internal malloc event:\n"
            "size: {}\n"
            "type: {}\n"
            "scope: {}",
            size, static_cast<int>(type), static_cast<int>(scope));
#endif
}

static void vk_internal_free([[maybe_unused]] void *, [[maybe_unused]] size_t size,
                             [[maybe_unused]] VkInternalAllocationType type,
                             [[maybe_unused]] VkSystemAllocationScope scope) {
#ifdef SURGE_DEBUG_MEMORY
  log_debug("Vk internal malloc free:\n"
            "size: {}\n"
            "type: {}\n"
            "scope: {}",
            size, static_cast<int>(type), static_cast<int>(scope));
#endif
}

// NOLINTNEXTLINE
static VkAllocationCallbacks vk_alloc_callbacks{.pUserData = nullptr,
                                                .pfnAllocation = vk_malloc,
                                                .pfnReallocation = vk_realloc,
                                                .pfnFree = vk_free,
                                                .pfnInternalAllocation = vk_internal_malloc,
                                                .pfnInternalFree = vk_internal_free};
#ifdef SURGE_USE_VK_VALIDATION_LAYERS
static VKAPI_ATTR auto VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT *callback, void *) -> VkBool32 {

  if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
      || severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {

    switch (type) {
    case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
      log_info("Vulkan Info (General): {}", callback->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
      log_info("Vulkan Info (Validation): {}", callback->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
      log_info("Vulkan Info (Performance): {}", callback->pMessage);
      break;
    default:
      break;
    }
    return VK_FALSE;

  } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {

    switch (type) {
    case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
      log_warn("Vulkan Warning (General): {}", callback->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
      log_warn("Vulkan Warning (Validation): {}", callback->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
      log_warn("Vulkan Warning (Performance): {}", callback->pMessage);
      break;
    default:
      break;
    }

    return VK_FALSE;

  } else {

    switch (type) {
    case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
      log_error("Vulkan Error (General): {}", callback->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
      log_error("Vulkan Error (Validation): {}", callback->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
      log_error("Vulkan Error (Performance): {}", callback->pMessage);
      break;
    default:
      break;
    }

    return VK_TRUE;
  }
}
#endif

auto surge::renderer::vk::create_swapchain(const config::renderer_attrs &r_attrs, context &ctx,
                                           u32 width, u32 height) noexcept
    -> tl::expected<swapchain_data, error> {
  swapchain_data swpc_data{};

  vkb::SwapchainBuilder swpc_builder{ctx.phys_device, ctx.device, ctx.surface};

  swpc_builder.set_desired_format(VkSurfaceFormatKHR{
      .format = VK_FORMAT_B8G8R8A8_UNORM, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR});

  // VSync
  if (r_attrs.vsync) {
    swpc_builder.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR);
  } else {
    swpc_builder.set_desired_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR);
  }

  swpc_builder.set_desired_extent(width, height);
  swpc_builder.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT);
  swpc_builder.set_allocation_callbacks(&vk_alloc_callbacks);

  const auto swpc_build_result{swpc_builder.build()};
  if (!swpc_build_result) {
    log_error("Error while creating Vulkan swapchain {}",
              string_VkResult(swpc_build_result.vk_result()));
    return tl::unexpected{error::vk_init_swapchain};
  } else {
    swpc_data.swapchain = swpc_build_result.value();
  }

  const auto get_swpc_img_result{swpc_data.swapchain.get_images()};
  if (!get_swpc_img_result) {
    log_error("Error while retrieving Vulkan swapchain images {}",
              string_VkResult(get_swpc_img_result.vk_result()));
    return tl::unexpected{error::vk_swapchain_imgs};
  } else {
    swpc_data.imgs = get_swpc_img_result.value();
  }

  const auto get_swpc_img_views_result{swpc_data.swapchain.get_image_views()};
  if (!get_swpc_img_views_result) {
    log_error("Error while retrieving Vulkan swapchain images {}",
              string_VkResult(get_swpc_img_views_result.vk_result()));
    return tl::unexpected{error::vk_swapchain_imgs_views};
  } else {
    swpc_data.imgs_views = get_swpc_img_views_result.value();
  }

  return swpc_data;
}

void surge::renderer::vk::destroy_swapchain(context &ctx, swapchain_data &swpc) noexcept {
  vkb::destroy_swapchain(swpc.swapchain);

  for (auto &img : swpc.imgs_views) {
    vkDestroyImageView(ctx.device.device, img, &vk_alloc_callbacks);
  }

  swpc.imgs.clear();
  swpc.imgs_views.clear();
}

// NOLINTNEXTLINE
auto surge::renderer::vk::command_pool_create_info(u32 queue_family_idx,
                                                   VkCommandPoolCreateFlags flags) noexcept
    -> VkCommandPoolCreateInfo {
  VkCommandPoolCreateInfo ci{};
  ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  ci.pNext = nullptr;
  ci.queueFamilyIndex = queue_family_idx;
  ci.flags = flags;
  return ci;
}

auto surge::renderer::vk::command_buffer_alloc_info(VkCommandPool pool, u32 count) noexcept
    -> VkCommandBufferAllocateInfo {
  VkCommandBufferAllocateInfo ai{};
  ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  ai.pNext = nullptr;
  ai.commandPool = pool;
  ai.commandBufferCount = count;
  ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  return ai;
}

auto surge::renderer::vk::command_buffer_begin_info(VkCommandBufferUsageFlags flags) {
  VkCommandBufferBeginInfo ci = {};
  ci.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  ci.pNext = nullptr;
  ci.pInheritanceInfo = nullptr;
  ci.flags = flags;
  return ci;
}

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

auto surge::renderer::vk::image_subresource_range(VkImageAspectFlags aspect_mask) noexcept
    -> VkImageSubresourceRange {
  VkImageSubresourceRange sum_img{};
  sum_img.aspectMask = aspect_mask;
  sum_img.baseMipLevel = 0;
  sum_img.levelCount = VK_REMAINING_MIP_LEVELS;
  sum_img.baseArrayLayer = 0;
  sum_img.layerCount = VK_REMAINING_ARRAY_LAYERS;
  return sum_img;
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

void surge::renderer::vk::transition_image(VkCommandBuffer cmd, VkImage image,
                                           VkImageLayout curr_layout, // NOLINT
                                           VkImageLayout new_layout) noexcept {
  // See https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples for finer sync
  // options

  VkImageAspectFlags aspect_mask{(new_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
                                     ? VK_IMAGE_ASPECT_DEPTH_BIT
                                     : VK_IMAGE_ASPECT_COLOR_BIT};

  VkImageMemoryBarrier2 img_barrier{};
  img_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
  img_barrier.pNext = nullptr;

  img_barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
  img_barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
  img_barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
  img_barrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

  img_barrier.oldLayout = curr_layout;
  img_barrier.newLayout = new_layout;

  img_barrier.image = image;
  img_barrier.subresourceRange = image_subresource_range(aspect_mask);

  VkDependencyInfo dep_info{};
  dep_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  dep_info.pNext = nullptr;

  dep_info.imageMemoryBarrierCount = 1;
  dep_info.pImageMemoryBarriers = &img_barrier;

  vkCmdPipelineBarrier2(cmd, &dep_info);
}

auto surge::renderer::vk::clear(context &ctx, const config::clear_color &w_ccl) noexcept
    -> std::optional<error> {
  // Aliases
  auto &dev{ctx.device};
  auto &graphics_queue{ctx.frm_data.graphics_queue};
  auto &swpc{ctx.swpc_data.swapchain.swapchain};
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

  // Make the swapchain image writeable
  transition_image(cmd_buff, swpc_img, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

  // Clear color
  VkClearColorValue clear_value{{w_ccl.r, w_ccl.g, w_ccl.b, w_ccl.a}};

  VkImageSubresourceRange clear_range{image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT)};
  vkCmdClearColorImage(cmd_buff, swpc_img, VK_IMAGE_LAYOUT_GENERAL, &clear_value, 1, &clear_range);

  // make the swapchain image into presentable mode
  transition_image(cmd_buff, swpc_img, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  // finalize the command buffer (we can no longer add commands, but it can now be executed)
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

auto surge::renderer::vk::init(const config::renderer_attrs &r_attrs,
                               const config::window_resolution &w_res,
                               const config::window_attrs &w_attrs) noexcept
    -> tl::expected<context, error> {
  context ctx{};
  log_info("Initializing Vulkan");

  /*****************************
   * Query required extensions *
   *****************************/
  log_info("Querying required Vulkan instance extensions");

  // GLFW extensions
  u32 glfw_extension_count{0};
  const auto glfw_extensions{glfwGetRequiredInstanceExtensions(&glfw_extension_count)};

  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    return tl::unexpected{error::glfw_vk_ext_retrive};
  }

  // NOLINTNEXTLINE
  vector<const char *> required_extensions{glfw_extensions, glfw_extensions + glfw_extension_count};

  // Debug handler (if validation layers are available)
#ifdef SURGE_USE_VK_VALIDATION_LAYERS
  required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

  // Printout
  for (const auto &ext : required_extensions) {
    log_info("Vulkan extension requested: {}", ext);
  }

  /************
   * Instance *
   ************/
  log_info("Creating Vulkan instance");

  vkb::InstanceBuilder builder{};
  builder.set_engine_name("SURGE - The Super Underrated Game Engine");
  builder.set_app_name(w_attrs.name.c_str());

  builder.set_app_version(VK_MAKE_API_VERSION(0, 1, 0, 0));
  builder.set_engine_version(
      VK_MAKE_API_VERSION(0, SURGE_VERSION_MAJOR, SURGE_VERSION_MINOR, SURGE_VERSION_PATCH));

  builder.require_api_version(1, 3, 0);
  builder.enable_extensions(required_extensions);

  builder.set_allocation_callbacks(&vk_alloc_callbacks);

  // Message handler and validation layers
#ifdef SURGE_USE_VK_VALIDATION_LAYERS
  log_info("Enabling Vulkan validation layers");
  builder.enable_layer("VK_LAYER_KHRONOS_validation");
  builder.request_validation_layers(true);
  builder.set_debug_callback(vk_debug_callback);
#endif

  const auto instance_result{builder.build()};

  if (!instance_result) {
    log_error("Error while initializing Vulkan instance {}",
              string_VkResult(instance_result.vk_result()));
    return tl::unexpected{error::vk_instance_init};
  } else {
    ctx.instance = instance_result.value();
  }

  /**********
   * Device *
   **********/

  // vulkan 1.3 features
  VkPhysicalDeviceVulkan13Features features{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
  features.dynamicRendering = true;
  features.synchronization2 = true;

  // vulkan 1.2 features
  VkPhysicalDeviceVulkan12Features features12{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
  features12.bufferDeviceAddress = true;
  features12.descriptorIndexing = true;

  // Surface
  const auto surface_result{glfwCreateWindowSurface(ctx.instance.instance, window::get_window_ptr(),
                                                    &vk_alloc_callbacks, &ctx.surface)};
  if (surface_result != VK_SUCCESS) {
    log_error("Window Vulkan surface creation failed: {}", string_VkResult(surface_result));
    return tl::unexpected{error::vk_surface_init};
  }

  // Physical device
  vkb::PhysicalDeviceSelector phys_dev_select{ctx.instance};
  phys_dev_select.set_minimum_version(1, 3);
  phys_dev_select.set_required_features_13(features);
  phys_dev_select.set_required_features_12(features12);
  phys_dev_select.set_surface(ctx.surface);

  const auto phys_dev_select_result{phys_dev_select.select()};

  if (!phys_dev_select_result) {
    log_error("Error while selecting Vulkan physical device {}",
              string_VkResult(phys_dev_select_result.vk_result()));
    return tl::unexpected{error::vk_phys_dev_select};
  } else {
    log_info("Selected Vulkan device {}", phys_dev_select_result.value().name);
    ctx.phys_device = phys_dev_select_result.value();
  }

  // Logical device
  vkb::DeviceBuilder logi_dev_build{phys_dev_select_result.value()};
  const auto device_build_result{logi_dev_build.build()};

  if (!device_build_result) {
    log_error("Error while creating Vulkan logical device {}",
              string_VkResult(device_build_result.vk_result()));
    return tl::unexpected{error::vk_logi_dev_select};
  } else {
    ctx.device = device_build_result.value();
  }

  /*************
   * Swapchain *
   *************/
  const auto swpc_create_result{create_swapchain(r_attrs, ctx, static_cast<u32>(w_res.width),
                                                 static_cast<u32>(w_res.height))};
  if (!swpc_create_result) {
    return tl::unexpected{swpc_create_result.error()};
  } else {
    ctx.swpc_data = swpc_create_result.value();
  }

  /**********
   * Queues *
   **********/
  const auto get_graph_queue_result{ctx.device.get_queue(vkb::QueueType::graphics)};
  if (!get_graph_queue_result) {
    log_error("Error while acquiring Vulkan Graphics Queue Vulkan from device {}",
              string_VkResult(get_graph_queue_result.vk_result()));
    return tl::unexpected{error::vk_graphics_queue_retrieve};
  } else {
    ctx.frm_data.graphics_queue = get_graph_queue_result.value();
  }

  const auto get_graph_queue_family_result{ctx.device.get_queue_index(vkb::QueueType::graphics)};
  if (!get_graph_queue_family_result) {
    log_error("Error while acquiring Vulkan Graphics Queue Vulkan from device {}",
              string_VkResult(get_graph_queue_family_result.vk_result()));
    return tl::unexpected{error::vk_graphics_queue_retrieve};
  } else {
    ctx.frm_data.graphics_queue_family = get_graph_queue_family_result.value();
  }

  /**********************
   * Command structures *
   **********************/

  // Resetable command pool
  auto cmd_pool_create_info{command_pool_create_info(
      ctx.frm_data.graphics_queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)};

  for (usize i = 0; i < ctx.frm_data.frame_overlap; i++) {
    auto result{vkCreateCommandPool(ctx.device.device, &cmd_pool_create_info, &vk_alloc_callbacks,
                                    &ctx.frm_data.command_pools[i])}; // NOLINT

    if (result != VK_SUCCESS) {
      log_error("Unable to create command pool: {}", string_VkResult(result));
      return tl::unexpected{error::vk_cmd_pool_creation};
    }

    // Command buffer
    // NOLINTNEXTLINE
    auto cmd_buffer{command_buffer_alloc_info(ctx.frm_data.command_pools[i], 1)};
    result = vkAllocateCommandBuffers(ctx.device.device, &cmd_buffer,
                                      &ctx.frm_data.command_buffers[i]); // NOLINT

    if (result != VK_SUCCESS) {
      log_error("Unable to create buffer: {}", string_VkResult(result));
      return tl::unexpected{error::vk_cmd_buffer_creation};
    }
  }

  /******************************
   * Synchronization structures *
   ******************************/
  auto fence_ci{fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT)};
  auto sem_ci{semaphore_create_info()};

  for (usize i = 0; i < ctx.frm_data.frame_overlap; i++) {
    auto result{vkCreateFence(ctx.device, &fence_ci, &vk_alloc_callbacks,
                              &ctx.frm_data.render_fences[i])}; // NOLINT

    if (result != VK_SUCCESS) {
      log_error("Unable to create fence: {}", string_VkResult(result));
      return tl::unexpected{error::vk_fence_creation};
    }

    result = vkCreateSemaphore(ctx.device, &sem_ci, &vk_alloc_callbacks,
                               &ctx.frm_data.render_semaphores[i]); // NOLINT

    if (result != VK_SUCCESS) {
      log_error("Unable to create renderer semaphore: {}", string_VkResult(result));
      return tl::unexpected{error::vk_semaphore_creation};
    }

    result = vkCreateSemaphore(ctx.device, &sem_ci, &vk_alloc_callbacks,
                               &ctx.frm_data.swpc_semaphores[i]); // NOLINT

    if (result != VK_SUCCESS) {
      log_error("Unable to create swapchain semaphore: {}", string_VkResult(result));
      return tl::unexpected{error::vk_fence_creation};
    }
  }

  return ctx;
}

void surge::renderer::vk::terminate(context &ctx) {
  log_info("Terminating Vulkan");

  log_info("Destroying frame data");

  // Wait until GPU idle
  vkWaitForFences(ctx.device, 2, ctx.frm_data.render_fences.data(), true, 1000000000);

  for (usize i = 0; i < ctx.frm_data.frame_overlap; i++) {
    // NOLINTNEXTLINE
    vkDestroySemaphore(ctx.device, ctx.frm_data.swpc_semaphores[i], &vk_alloc_callbacks);

    // NOLINTNEXTLINE
    vkDestroySemaphore(ctx.device, ctx.frm_data.render_semaphores[i], &vk_alloc_callbacks);

    // NOLINTNEXTLINE
    vkDestroyFence(ctx.device, ctx.frm_data.render_fences[i], &vk_alloc_callbacks);

    // NOLINTNEXTLINE
    vkFreeCommandBuffers(ctx.device, ctx.frm_data.command_pools[i], 1,
                         &ctx.frm_data.command_buffers[i]); // NOLINT

    // NOLINTNEXTLINE
    vkDestroyCommandPool(ctx.device, ctx.frm_data.command_pools[i], &vk_alloc_callbacks);
  }

  log_info("Destroying swapchain");
  destroy_swapchain(ctx, ctx.swpc_data);

  log_info("Destroying window surface");
  vkb::destroy_surface(ctx.instance, ctx.surface);

  log_info("Destroying logical device");
  vkb::destroy_device(ctx.device);

  log_info("Destroying instance");
  vkb::destroy_instance(ctx.instance);
}