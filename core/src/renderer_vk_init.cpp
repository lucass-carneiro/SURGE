#include "renderer_vk_init.hpp"

#include "logging.hpp"
#include "renderer_vk_command.hpp"
#include "renderer_vk_debug.hpp"
#include "renderer_vk_malloc.hpp"
#include "renderer_vk_sync.hpp"
#include "window.hpp"

#include <algorithm>
#include <vulkan/vk_enum_string_helper.h>

auto surge::renderer::vk::get_required_extensions() noexcept
    -> tl::expected<vector<const char *>, error> {
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
    log_info("Vulkan extension required: {}", ext);
  }

  return required_extensions;
}

#ifdef SURGE_USE_VK_VALIDATION_LAYERS
auto surge::renderer::vk::get_required_validation_layers() noexcept
    -> tl::expected<vector<const char *>, error> {
  using std::strcmp;

  log_info("Cheking available validation layers");

  u32 layer_count{0};
  auto result{vkEnumerateInstanceLayerProperties(&layer_count, nullptr)};

  if (result != VK_SUCCESS) {
    log_error("Unable query available validation layers: {}", string_VkResult(result));
    return tl::unexpected{error::vk_val_layer_query};
  }

  vector<VkLayerProperties> available_layers(layer_count);
  result = vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

  if (result != VK_SUCCESS) {
    log_error("Unable query available validation layers: {}", string_VkResult(result));
    return tl::unexpected{error::vk_val_layer_query};
  }

  for (const auto &layer_properties : available_layers) {
    log_info("Available validation layer: {}", layer_properties.layerName);
  }

  vector<const char *> required_layers{};
  required_layers.push_back("VK_LAYER_KHRONOS_validation");

  for (const auto &layer_name : required_layers) {
    bool layer_found{false};

    for (const auto &layer_properties : available_layers) {
      if (strcmp(layer_name, layer_properties.layerName) == 0) {
        layer_found = true;
        break;
      }
    }

    if (!layer_found) {
      return tl::unexpected{error::vk_val_layer_missing};
    }
  }

  return required_layers;
}
#endif

auto surge::renderer::vk::create_instance() noexcept -> tl::expected<VkInstance, error> {
  log_info("Creating instance");

  const auto required_extensions{get_required_extensions()};
  if (!required_extensions) {
    return tl::unexpected{required_extensions.error()};
  }

#ifdef SURGE_USE_VK_VALIDATION_LAYERS
  const auto required_layers{get_required_validation_layers()};
  if (!required_layers) {
    return tl::unexpected{required_layers.error()};
  }

  auto dbg_msg_ci{dbg_msg_create_info()};
#endif

  VkApplicationInfo app_info{
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pNext = nullptr,
      .pApplicationName = "SURGE Player",
      .applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
      .pEngineName = "SURGE",
      .engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
      .apiVersion = VK_API_VERSION_1_3,
  };

  VkInstanceCreateInfo create_info{.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#ifdef SURGE_USE_VK_VALIDATION_LAYERS
                                   .pNext = &dbg_msg_ci,
#else
                                   .pNext = nullptr,
#endif
                                   .flags = 0,
                                   .pApplicationInfo = &app_info,
#ifdef SURGE_USE_VK_VALIDATION_LAYERS
                                   .enabledLayerCount = static_cast<u32>(required_layers->size()),
                                   .ppEnabledLayerNames = required_layers->data(),
#else
                                   .enabledLayerCount = 0,
                                   .ppEnabledLayerNames = nullptr,
#endif
                                   .enabledExtensionCount
                                   = static_cast<u32>(required_extensions->size()),
                                   .ppEnabledExtensionNames = required_extensions->data()};

  VkInstance instance{};
  const auto result{vkCreateInstance(&create_info, get_alloc_callbacks(), &instance)};
  if (result != VK_SUCCESS) {
    log_error("Unable initialize vulkan instance: {}", string_VkResult(result));
    return tl::unexpected{error::vk_instance_init};
  }

  return instance;
}

auto surge::renderer::vk::find_queue_families(VkPhysicalDevice phys_dev) noexcept
    -> queue_family_indices {
  queue_family_indices idxs{};

  uint32_t queue_family_count{0};
  vkGetPhysicalDeviceQueueFamilyProperties(phys_dev, &queue_family_count, nullptr);

  vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(phys_dev, &queue_family_count, queue_families.data());

  for (u32 i = 0; const auto &family : queue_families) {
    if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      idxs.graphics_family = i;
    }

    if (family.queueFlags & VK_QUEUE_TRANSFER_BIT) {
      idxs.transfer_family = i;
    }

    if (family.queueFlags & VK_QUEUE_COMPUTE_BIT) {
      idxs.compute_family = i;
    }

    i++;
  }

  return idxs;
}

auto surge::renderer::vk::get_required_device_extensions(VkPhysicalDevice phys_dev) noexcept
    -> tl::expected<vector<const char *>, error> {
  using std::strcmp;

  log_info("Cheking device extensions");

  u32 extension_count{0};
  auto result{vkEnumerateDeviceExtensionProperties(phys_dev, nullptr, &extension_count, nullptr)};

  if (result != VK_SUCCESS) {
    log_error("Unable retrieve device extension properties: {}", string_VkResult(result));
    return tl::unexpected{error::vk_phys_dev_ext_enum};
  }

  vector<VkExtensionProperties> available_extensions(extension_count);
  result = vkEnumerateDeviceExtensionProperties(phys_dev, nullptr, &extension_count,
                                                available_extensions.data());

  if (result != VK_SUCCESS) {
    log_error("Unable retrieve device extension properties: {}", string_VkResult(result));
    return tl::unexpected{error::vk_phys_dev_ext_enum};
  }

  vector<const char *> required_extensions{};
  required_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

  for (const auto &ext_name : required_extensions) {
    bool layer_found{false};

    for (const auto &ext_properties : available_extensions) {
      if (strcmp(ext_name, ext_properties.extensionName) == 0) {
        layer_found = true;
        break;
      }
    }

    if (!layer_found) {
      return tl::unexpected{error::vk_phys_dev_ext_missing};
    }
  }

  return required_extensions;
}

auto surge::renderer::vk::is_device_suitable(VkPhysicalDevice phys_dev) noexcept -> bool {
  log_info("Cheking device suitability");

  VkPhysicalDeviceProperties dev_prop{};
  vkGetPhysicalDeviceProperties(phys_dev, &dev_prop);

  VkPhysicalDeviceVulkan13Features features_13{};
  features_13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

  VkPhysicalDeviceVulkan12Features features_12{};
  features_12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  features_12.pNext = &features_13;

  VkPhysicalDeviceFeatures2 features{};
  features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  features.pNext = &features_12;

  vkGetPhysicalDeviceFeatures2(phys_dev, &features);

  const auto has_required_features{features_12.bufferDeviceAddress && features_12.descriptorIndexing
                                   && features_13.dynamicRendering && features_13.synchronization2};

  const auto idxs{find_queue_families(phys_dev)};
  const auto has_all_queues{idxs.graphics_family.has_value() && idxs.transfer_family.has_value()
                            && idxs.compute_family.has_value()};

  const auto has_device_extensions{get_required_device_extensions(phys_dev).has_value()};

  if (has_required_features && has_all_queues && has_device_extensions) {
    log_info("Suitable device found: {}", dev_prop.deviceName);
    return true;
  } else {
    return false;
  }
}

auto surge::renderer::vk::select_physical_device(VkInstance instance) noexcept
    -> tl::expected<VkPhysicalDevice, error> {
  log_info("Selecting first suitable physical device");

  uint32_t dev_count{0};
  auto result{vkEnumeratePhysicalDevices(instance, &dev_count, nullptr)};

  if (result != VK_SUCCESS) {
    log_error("Unable enumerate physical devices: {}", string_VkResult(result));
    return tl::unexpected{error::vk_phys_dev_enum};
  }

  std::vector<VkPhysicalDevice> phys_devs(dev_count);
  result = vkEnumeratePhysicalDevices(instance, &dev_count, phys_devs.data());

  if (result != VK_SUCCESS) {
    log_error("Unable enumerate physical devices: {}", string_VkResult(result));
    return tl::unexpected{error::vk_phys_dev_enum};
  }

  for (const auto &phys_dev : phys_devs) {
    if (is_device_suitable(phys_dev)) {
      return phys_dev;
    }
  }

  return tl::unexpected{error::vk_phys_dev_no_suitable};
}

auto surge::renderer::vk::create_logical_device(VkPhysicalDevice phys_dev) noexcept
    -> tl::expected<VkDevice, error> {
  log_info("Creating logical device");

  // Extensions
  const auto device_extensions{get_required_device_extensions(phys_dev)};
  if (!device_extensions) {
    return tl::unexpected{device_extensions.error()};
  }

  // vulkan 1.3 features
  VkPhysicalDeviceVulkan13Features features_13{};
  features_13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
  features_13.dynamicRendering = true;
  features_13.synchronization2 = true;

  // vulkan 1.2 features
  VkPhysicalDeviceVulkan12Features features_12{};
  features_12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  features_12.pNext = &features_13;
  features_12.bufferDeviceAddress = true;
  features_12.descriptorIndexing = true;

  VkPhysicalDeviceFeatures2 features{};
  features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  features.pNext = &features_12;

  const auto indices{find_queue_families(phys_dev)};

  const float queue_priority{1.0f};

  // value_or(0) never returns 0 here because the selected device is only suitable if all queues
  // are found.
  VkDeviceQueueCreateInfo graphics_queue_ci{.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                            .pNext = nullptr,
                                            .flags = 0,
                                            .queueFamilyIndex = indices.graphics_family.value_or(0),
                                            .queueCount = 1,
                                            .pQueuePriorities = &queue_priority};

  VkDeviceQueueCreateInfo transfer_queue_ci{.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                            .pNext = nullptr,
                                            .flags = 0,
                                            .queueFamilyIndex = indices.transfer_family.value_or(0),
                                            .queueCount = 1,
                                            .pQueuePriorities = &queue_priority};

  VkDeviceQueueCreateInfo compute_queue_ci{.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                           .pNext = nullptr,
                                           .flags = 0,
                                           .queueFamilyIndex = indices.compute_family.value_or(0),
                                           .queueCount = 1,
                                           .pQueuePriorities = &queue_priority};

  std::array<VkDeviceQueueCreateInfo, 3> queue_create_infos{graphics_queue_ci, transfer_queue_ci,
                                                            compute_queue_ci};

  VkDeviceCreateInfo create_info{
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = &features,
      .flags = 0,
      .queueCreateInfoCount = static_cast<u32>(queue_create_infos.size()),
      .pQueueCreateInfos = queue_create_infos.data(),
      .enabledLayerCount = 0,
      .ppEnabledLayerNames = nullptr,
      .enabledExtensionCount = static_cast<u32>(device_extensions->size()),
      .ppEnabledExtensionNames = device_extensions->data(),
      .pEnabledFeatures = nullptr};

  VkDevice log_dev{};
  const auto result{vkCreateDevice(phys_dev, &create_info, get_alloc_callbacks(), &log_dev)};

  if (result != VK_SUCCESS) {
    log_error("Unable create logical device: {}", string_VkResult(result));
    return tl::unexpected{error::vk_log_dev_create};
  } else {
    return log_dev;
  }
}

auto surge::renderer::vk::create_window_surface(VkInstance instance) noexcept
    -> tl::expected<VkSurfaceKHR, error> {
  log_info("Creating window surface");

  VkSurfaceKHR surface{};
  const auto result{
      glfwCreateWindowSurface(instance, window::get_window_ptr(), get_alloc_callbacks(), &surface)};

  if (result != VK_SUCCESS) {
    log_error("Window Vulkan surface creation failed: {}", string_VkResult(result));
    return tl::unexpected{error::vk_surface_init};
  } else {
    return surface;
  }
}

auto surge::renderer::vk::get_queue_handles(VkPhysicalDevice phys_dev, VkDevice log_dev,
                                            VkSurfaceKHR surface) noexcept
    -> tl::expected<queue_handles, error> {
  log_info("Getting queue handles");

  queue_handles handles{};

  const auto idxs{find_queue_families(phys_dev)};

  VkBool32 graphics_present_suport{false};
  const auto result{vkGetPhysicalDeviceSurfaceSupportKHR(phys_dev, idxs.graphics_family.value_or(0),
                                                         surface, &graphics_present_suport)};
  if (result != VK_SUCCESS) {
    log_error("Unable to query graphics queue for presentation support: {}",
              string_VkResult(result));
    return tl::unexpected{error::vk_surface_present_query};
  }

  if (!graphics_present_suport) {
    log_error("The graphics queue provided by the device does not support presentation");
    return tl::unexpected{error::vk_surface_present_unable};
  }

  // value_or(0) never returns 0 here because the selected device is only suitable if all queues
  // are found.
  handles.graphics_idx = idxs.graphics_family.value_or(0);
  handles.transfer_idx = idxs.transfer_family.value_or(0);
  handles.compute_idx = idxs.compute_family.value_or(0);

  vkGetDeviceQueue(log_dev, handles.graphics_idx, 0, &(handles.graphics));
  vkGetDeviceQueue(log_dev, handles.transfer_idx, 0, &(handles.transfer));
  vkGetDeviceQueue(log_dev, handles.compute_idx, 0, &(handles.compute));

  return handles;
}

auto surge::renderer::vk::create_swapchain(
    VkPhysicalDevice phys_dev, VkDevice log_dev, VkSurfaceKHR surface,
    const config::renderer_attrs &r_attrs, u32 width,
    u32 height) noexcept -> tl::expected<swapchain_data, error> {
  log_info("Creating swapchain");

  using std::clamp;

  swapchain_data swpc_data{};

  VkSurfaceCapabilitiesKHR surface_capabilities{};
  auto result{vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys_dev, surface, &surface_capabilities)};

  if (result != VK_SUCCESS) {
    log_error("Unable to query physical device surface capabilities: {}", string_VkResult(result));
    return tl::unexpected{error::vk_swapchain_query};
  }

  const auto img_format{VK_FORMAT_B8G8R8A8_UNORM};
  const auto img_colorspace{VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
  const auto img_usage{VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT};

  VkPresentModeKHR present_mode{};

  // VSync
  if (r_attrs.vsync) {
    present_mode = VK_PRESENT_MODE_FIFO_KHR;
  } else {
    present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
  }

  VkExtent2D extent{clamp(width, surface_capabilities.minImageExtent.width,
                          surface_capabilities.maxImageExtent.width),
                    clamp(height, surface_capabilities.minImageExtent.height,
                          surface_capabilities.maxImageExtent.height)};
  swpc_data.extent = extent;

  uint32_t image_count{surface_capabilities.minImageCount + 1};

  if (surface_capabilities.maxImageCount > 0 && image_count > surface_capabilities.maxImageCount) {
    image_count = surface_capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR swpc_create_info{.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                                            .pNext = nullptr,
                                            .flags = 0,
                                            .surface = surface,
                                            .minImageCount = image_count,
                                            .imageFormat = img_format,
                                            .imageColorSpace = img_colorspace,
                                            .imageExtent = extent,
                                            .imageArrayLayers = 1,
                                            .imageUsage = img_usage,
                                            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
                                            .queueFamilyIndexCount = 0,
                                            .pQueueFamilyIndices = nullptr,
                                            .preTransform = surface_capabilities.currentTransform,
                                            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                                            .presentMode = present_mode,
                                            .clipped = VK_TRUE,
                                            .oldSwapchain = VK_NULL_HANDLE};

  result = vkCreateSwapchainKHR(log_dev, &swpc_create_info, get_alloc_callbacks(),
                                &swpc_data.swapchain);
  if (result != VK_SUCCESS) {
    log_error("Unable to create swapchain: {}", string_VkResult(result));
    return tl::unexpected{error::vk_init_swapchain};
  }

  result = vkGetSwapchainImagesKHR(log_dev, swpc_data.swapchain, &image_count, nullptr);
  if (result != VK_SUCCESS) {
    log_error("Unable to get swapchain images: {}", string_VkResult(result));
    return tl::unexpected{error::vk_swapchain_imgs};
  }

  swpc_data.imgs.resize(image_count);
  result
      = vkGetSwapchainImagesKHR(log_dev, swpc_data.swapchain, &image_count, swpc_data.imgs.data());
  if (result != VK_SUCCESS) {
    log_error("Unable to get swapchain images: {}", string_VkResult(result));
    return tl::unexpected{error::vk_swapchain_imgs};
  }

  const VkComponentMapping img_view_components{
      VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
      VK_COMPONENT_SWIZZLE_IDENTITY};

  const VkImageSubresourceRange img_view_sub_range{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                                   .baseMipLevel = 0,
                                                   .levelCount = 1,
                                                   .baseArrayLayer = 0,
                                                   .layerCount = 1};

  for (const auto &img : swpc_data.imgs) {
    VkImageViewCreateInfo img_view_ci{.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                                      .pNext = nullptr,
                                      .flags = 0,
                                      .image = img,
                                      .viewType = VK_IMAGE_VIEW_TYPE_2D,
                                      .format = img_format,
                                      .components = img_view_components,
                                      .subresourceRange = img_view_sub_range};

    VkImageView img_view{};
    result = vkCreateImageView(log_dev, &img_view_ci, get_alloc_callbacks(), &img_view);
    if (result != VK_SUCCESS) {
      log_error("Unable to create swapchain image view: {}", string_VkResult(result));
      return tl::unexpected{error::vk_swapchain_imgs_views};
    } else {
      swpc_data.imgs_views.push_back(img_view);
    }
  }

  return swpc_data;
}

auto surge::renderer::vk::create_frame_data(VkDevice device, u32 graphics_queue_idx) noexcept
    -> tl::expected<frame_data, error> {
  frame_data frm_data{};

  /******************************
   * Command structures *
   ******************************/
  auto cmd_pool_create_info{command_pool_create_info(
      graphics_queue_idx, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)};

  for (usize i = 0; i < frm_data.frame_overlap; i++) {
    auto result{vkCreateCommandPool(device, &cmd_pool_create_info, get_alloc_callbacks(),
                                    &frm_data.command_pools[i])}; // NOLINT

    if (result != VK_SUCCESS) {
      log_error("Unable to create command pool: {}", string_VkResult(result));
      return tl::unexpected{error::vk_cmd_pool_creation};
    }

    // Command buffer
    // NOLINTNEXTLINE
    auto cmd_buffer{command_buffer_alloc_info(frm_data.command_pools[i], 1)};
    result = vkAllocateCommandBuffers(device, &cmd_buffer,
                                      &frm_data.command_buffers[i]); // NOLINT

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

  for (usize i = 0; i < frm_data.frame_overlap; i++) {
    auto result{vkCreateFence(device, &fence_ci, get_alloc_callbacks(),
                              &frm_data.render_fences[i])}; // NOLINT

    if (result != VK_SUCCESS) {
      log_error("Unable to create fence: {}", string_VkResult(result));
      return tl::unexpected{error::vk_fence_creation};
    }

    result = vkCreateSemaphore(device, &sem_ci, get_alloc_callbacks(),
                               &frm_data.render_semaphores[i]); // NOLINT

    if (result != VK_SUCCESS) {
      log_error("Unable to create renderer semaphore: {}", string_VkResult(result));
      return tl::unexpected{error::vk_semaphore_creation};
    }

    result = vkCreateSemaphore(device, &sem_ci, get_alloc_callbacks(),
                               &frm_data.swpc_semaphores[i]); // NOLINT

    if (result != VK_SUCCESS) {
      log_error("Unable to create swapchain semaphore: {}", string_VkResult(result));
      return tl::unexpected{error::vk_fence_creation};
    }
  }

  return frm_data;
}

void surge::renderer::vk::destroy_frame_data(context &ctx) noexcept {
  log_info("Destroying frame data");
  for (usize i = 0; i < ctx.frm_data.frame_overlap; i++) {
    // NOLINTNEXTLINE
    vkDestroySemaphore(ctx.log_dev, ctx.frm_data.swpc_semaphores[i], get_alloc_callbacks());

    // NOLINTNEXTLINE
    vkDestroySemaphore(ctx.log_dev, ctx.frm_data.render_semaphores[i], get_alloc_callbacks());

    // NOLINTNEXTLINE
    vkDestroyFence(ctx.log_dev, ctx.frm_data.render_fences[i], get_alloc_callbacks());

    // NOLINTNEXTLINE
    vkFreeCommandBuffers(ctx.log_dev, ctx.frm_data.command_pools[i], 1,
                         &ctx.frm_data.command_buffers[i]); // NOLINT

    // NOLINTNEXTLINE
    vkDestroyCommandPool(ctx.log_dev, ctx.frm_data.command_pools[i], get_alloc_callbacks());
  }
}