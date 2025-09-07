#include "sc_vulkan/sc_vulkan_init.hpp"

#include "sc_integer_types.hpp"
#include "sc_logging.hpp"
#include "sc_vulkan/sc_vulkan_command.hpp"
#include "sc_vulkan/sc_vulkan_malloc.hpp"
#include "sc_vulkan/sc_vulkan_sync.hpp"

#include <algorithm>

auto surge::renderer::vk::get_supported_api_version() -> Result<u32> {
  log_info("Querying available Vulkan API version");

  u32 vulkan_api_version{0};
  const auto result{vkEnumerateInstanceVersion(&vulkan_api_version)};

  if (result != VK_SUCCESS) {
    log_error("Unable query Vulkan API version:");
    return Err{Error::vk_api_version_query};
  }

  log_info("Vulkan API suported in this system: {}.{}.{}-{}",
           VK_API_VERSION_MAJOR(vulkan_api_version), VK_API_VERSION_MINOR(vulkan_api_version),
           VK_API_VERSION_PATCH(vulkan_api_version), VK_API_VERSION_VARIANT(vulkan_api_version));

  return vulkan_api_version;
}

auto surge::renderer::vk::get_required_instance_extensions()
    -> Result<containers::mimalloc::Vector<const char *>> {
  log_info("Querying required Vulkan instance extensions");

  // GLFW extensions
  u32 glfw_extension_count{0};
  const auto glfw_extensions{glfwGetRequiredInstanceExtensions(&glfw_extension_count)};

  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    return Err{Error::glfw_vk_ext_retrive};
  }

  // NOLINTNEXTLINE
  containers::mimalloc::Vector<const char *> required_extensions(
      glfw_extensions, glfw_extensions + glfw_extension_count);

  // Debug handler (if validation layers are available)
#ifdef SURGE_USE_VK_VALIDATION_LAYERS
  required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

  // Printout
  for (const auto &ext : required_extensions) {
    log_info("Vulkan instance extension required: {}", ext);
  }

  return required_extensions;
}

#ifdef SURGE_USE_VK_VALIDATION_LAYERS
auto surge::renderer::vk::get_required_validation_layers()
    -> Result<containers::mimalloc::Vector<const char *>> {
  using std::strcmp;

  log_info("Cheking available validation layers");

  u32 layer_count{0};
  auto result{vkEnumerateInstanceLayerProperties(&layer_count, nullptr)};

  if (result != VK_SUCCESS) {
    log_error("Unable to query available validation layers:");
    return Err{Error::vk_val_layer_query};
  }

  containers::mimalloc::Vector<VkLayerProperties> available_layers(layer_count);
  result = vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

  if (result != VK_SUCCESS) {
    log_error("Unable to query available validation layers:");
    return Err{Error::vk_val_layer_query};
  }

  for (const auto &layer_properties : available_layers) {
    log_info("Available validation layer: {}", layer_properties.layerName);
  }

  containers::mimalloc::Vector<const char *> required_layers{};
  required_layers.push_back("VK_LAYER_KHRONOS_validation");

  for (const auto &layer_name : required_layers) {
    log_info("Requiring validation layer {}", layer_name);

    bool layer_found{false};

    for (const auto &layer_properties : available_layers) {
      if (strcmp(layer_name, layer_properties.layerName) == 0) {
        layer_found = true;
        break;
      }
    }

    if (!layer_found) {
      log_error("Unable to find validation layer {}", layer_name);
      return Err{Error::vk_val_layer_missing};
    }
  }

  return required_layers;
}
#endif

#ifdef SURGE_USE_VK_VALIDATION_LAYERS
auto surge::renderer::vk::build_instance(
    const containers::mimalloc::Vector<const char *> &required_instance_extensions,
    const containers::mimalloc::Vector<const char *> &required_validation_layers,
    const VkDebugUtilsMessengerCreateInfoEXT &dbg_msg_ci) -> Result<VkInstance> {
  log_info("Creating Vulkan Instance");

  VkApplicationInfo app_info{
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pNext = nullptr,
      .pApplicationName = "SURGE Player",
      .applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
      .pEngineName = "SURGE",
      .engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
      .apiVersion = VK_API_VERSION_1_3,
  };

  VkInstanceCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pNext = &dbg_msg_ci;
  create_info.flags = 0;
  create_info.pApplicationInfo = &app_info;
  create_info.enabledLayerCount = static_cast<u32>(required_validation_layers.size());
  create_info.ppEnabledLayerNames = required_validation_layers.data();
  create_info.enabledExtensionCount = static_cast<u32>(required_instance_extensions.size());
  create_info.ppEnabledExtensionNames = required_instance_extensions.data();

  VkInstance instance{};
  const auto result{vkCreateInstance(&create_info, get_alloc_callbacks(), &instance)};
  if (result != VK_SUCCESS) {
    log_error("Unable initialize vulkan instance:");
    return Err{Error::vk_instance_init};
  }

  log_info("Vulkan instance created");

  return instance;
}
#else
auto surge::renderer::vk::build_instance(
    const containers::mimalloc::Vector<const char *> &required_instance_extensions)
    -> Result<VkInstance> {
  log_info("Creating Vulkan Instance");

  VkApplicationInfo app_info{
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pNext = nullptr,
      .pApplicationName = "SURGE Player",
      .applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
      .pEngineName = "SURGE",
      .engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
      .apiVersion = VK_API_VERSION_1_3,
  };

  VkInstanceCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pNext = nullptr;
  create_info.flags = 0;
  create_info.pApplicationInfo = &app_info;
  create_info.enabledLayerCount = 0;
  create_info.ppEnabledLayerNames = nullptr;
  create_info.enabledExtensionCount = static_cast<u32>(required_instance_extensions.size());
  create_info.ppEnabledExtensionNames = required_instance_extensions.data();

  VkInstance instance{};
  const auto result{vkCreateInstance(&create_info, get_alloc_callbacks(), &instance)};
  if (result != VK_SUCCESS) {
    log_error("Unable initialize vulkan instance:");
    return Err{Error::vk_instance_init};
  }

  log_info("Vulkan instance created");

  return instance;
}
#endif

auto surge::renderer::vk::get_available_physical_devices(VkInstance instance)
    -> Result<containers::mimalloc::Vector<VkPhysicalDevice>> {
  uint32_t dev_count{0};
  auto result{vkEnumeratePhysicalDevices(instance, &dev_count, nullptr)};

  if (result != VK_SUCCESS) {
    log_error("Unable enumerate physical devices:");
    return Err{Error::vk_phys_dev_enum};
  }

  containers::mimalloc::Vector<VkPhysicalDevice> phys_devs(dev_count);
  result = vkEnumeratePhysicalDevices(instance, &dev_count, phys_devs.data());

  if (result != VK_SUCCESS) {
    log_error("Unable enumerate physical devices:");
    return Err{Error::vk_phys_dev_enum};
  }

  return phys_devs;
}

auto surge::renderer::vk::get_available_device_features(VkPhysicalDevice phys_dev)
    -> std::tuple<VkPhysicalDeviceVulkan12Features, VkPhysicalDeviceVulkan13Features> {
  VkPhysicalDeviceVulkan13Features features_13{};
  features_13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
  features_13.pNext = nullptr;

  VkPhysicalDeviceVulkan12Features features_12{};
  features_12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  features_12.pNext = &features_13;

  VkPhysicalDeviceFeatures2 features{};
  features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  features.pNext = &features_12;

  vkGetPhysicalDeviceFeatures2(phys_dev, &features);

  return std::make_tuple(features_12, features_13);
}

auto surge::renderer::vk::get_available_device_extensions(VkPhysicalDevice phys_dev)
    -> Result<containers::mimalloc::Vector<VkExtensionProperties>> {
  u32 extension_count{0};
  auto result{vkEnumerateDeviceExtensionProperties(phys_dev, nullptr, &extension_count, nullptr)};

  if (result != VK_SUCCESS) {
    log_error("Unable retrieve available device extensions.");
    return Err{Error::vk_phys_dev_ext_enum};
  }

  containers::mimalloc::Vector<VkExtensionProperties> available_extensions(extension_count);
  result = vkEnumerateDeviceExtensionProperties(phys_dev, nullptr, &extension_count,
                                                available_extensions.data());

  if (result != VK_SUCCESS) {
    log_error("Unable retrieve device extension properties:");
    return Err{Error::vk_phys_dev_ext_enum};
  }

  return available_extensions;
}

auto surge::renderer::vk::get_available_device_queue_families(VkPhysicalDevice phys_dev)
    -> containers::mimalloc::Vector<VkQueueFamilyProperties> {
  uint32_t queue_family_count{0};
  vkGetPhysicalDeviceQueueFamilyProperties(phys_dev, &queue_family_count, nullptr);

  containers::mimalloc::Vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(phys_dev, &queue_family_count, queue_families.data());

  return queue_families;
}

auto surge::renderer::vk::get_required_device_features()
    -> std::tuple<VkPhysicalDeviceVulkan12Features, VkPhysicalDeviceVulkan13Features> {
  // vulkan 1.3 features
  VkPhysicalDeviceVulkan13Features features_13{};
  features_13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
  features_13.pNext = nullptr;
  features_13.dynamicRendering = true;
  features_13.synchronization2 = true;

  // vulkan 1.2 features
  VkPhysicalDeviceVulkan12Features features_12{};
  features_12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  features_12.pNext = nullptr;
  features_12.bufferDeviceAddress = true;
  features_12.descriptorIndexing = true;
  features_12.shaderSampledImageArrayNonUniformIndexing = true;
  features_12.runtimeDescriptorArray = true;
  features_12.descriptorBindingVariableDescriptorCount = true;
  features_12.descriptorBindingPartiallyBound = true;

  return std::make_tuple(features_12, features_13);
}

auto surge::renderer::vk::get_required_device_extensions()
    -> containers::mimalloc::Vector<const char *> {
  containers::mimalloc::Vector<const char *> required_extensions{};

  required_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
  required_extensions.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

  return required_extensions;
}

auto surge::renderer::vk::get_required_device_queue_families()
    -> containers::mimalloc::Vector<VkQueueFlagBits> {
  containers::mimalloc::Vector<VkQueueFlagBits> queue_families;

  queue_families.push_back(VK_QUEUE_GRAPHICS_BIT);
  queue_families.push_back(VK_QUEUE_TRANSFER_BIT);
  queue_families.push_back(VK_QUEUE_COMPUTE_BIT);

  return queue_families;
}

auto surge::renderer::vk::device_has_required_features(VkPhysicalDevice phys_dev) -> bool {
  const auto [av_features_12, av_features_13] = get_available_device_features(phys_dev);

  const auto has_features_12{av_features_12.bufferDeviceAddress && av_features_12.descriptorIndexing
                             && av_features_12.shaderSampledImageArrayNonUniformIndexing
                             && av_features_12.runtimeDescriptorArray
                             && av_features_12.descriptorBindingVariableDescriptorCount
                             && av_features_12.descriptorBindingPartiallyBound};

  const auto has_features_13{av_features_13.dynamicRendering && av_features_13.synchronization2};

  return has_features_12 && has_features_13;
}

auto surge::renderer::vk::device_has_required_extensions(VkPhysicalDevice phys_dev) -> bool {
  const auto av_dev_ext{get_available_device_extensions(phys_dev)};

  if (!av_dev_ext) {
    return false;
  }

  const auto rq_dev_ext{get_required_device_extensions()};

  for (const auto &rq_ext : rq_dev_ext) {
    bool ext_found{false};

    for (const auto &av_ext : *av_dev_ext) {
      if (strcmp(rq_ext, av_ext.extensionName) == 0) {
        ext_found = true;
        break;
      }
    }

    if (!ext_found) {
      log_error("Extension {} not available on device", rq_ext);
      return false;
    }
  }

  return true;
}

auto surge::renderer::vk::device_has_required_queue_families(VkPhysicalDevice phys_dev) -> bool {
  const auto av_queue_families{get_available_device_queue_families(phys_dev)};
  const auto rq_queue_families{get_required_device_queue_families()};

  // Check if the device has all queue families
  for (const auto &rq_family : rq_queue_families) {
    bool found{false};

    for (const auto &av_queue_family : av_queue_families) {
      if (rq_family & av_queue_family.queueFlags) {
        found = true;
        break;
      }
    }

    if (!found) {
      return false;
    }
  }

  return true;
}

auto surge::renderer::vk::get_queue_family_indices(
    const containers::mimalloc::Vector<VkQueueFamilyProperties> &queue_families)
    -> QueueFamilyIndices {
  QueueFamilyIndices indices{};

  for (u32 i = 0; i < queue_families.size(); i++) {
    if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphics_family = i;
    }

    if (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
      indices.transfer_family = i;
    }
    if (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
      indices.compute_family = i;
    }
  }

  return indices;
}

auto surge::renderer::vk::select_physical_device(VkInstance instance) -> Result<VkPhysicalDevice> {
  log_info("Selecting first suitable physical device");

  const auto phys_devs{get_available_physical_devices(instance)};
  if (!phys_devs) {
    return Err{phys_devs.error()};
  }

  for (const auto &phys_dev : *phys_devs) {
    if (is_device_suitable(phys_dev)) {
      return phys_dev;
    }
  }

  log_error("Unable to find suitable Vulkan Physical Device");
  return Err{Error::vk_phys_dev_no_suitable};
}

auto surge::renderer::vk::is_device_suitable(VkPhysicalDevice phys_dev) -> bool {
  VkPhysicalDeviceProperties dev_props{};
  vkGetPhysicalDeviceProperties(phys_dev, &dev_props);

  log_info("Cheking suitability of {}", dev_props.deviceName);

  const auto has_required_features{device_has_required_features(phys_dev)};
  const auto has_device_extensions{device_has_required_extensions(phys_dev)};
  const auto has_queue_families{device_has_required_queue_families(phys_dev)};

  // TODO: This is probably overly restrictive and should be relaxed in the future.
  const auto queue_families{get_available_device_queue_families(phys_dev)};
  const auto indices{get_queue_family_indices(queue_families)};

  const auto non_repeated_queue_family_indices{indices.graphics_family != indices.compute_family
                                               && indices.graphics_family != indices.transfer_family
                                               && indices.compute_family
                                                      != indices.transfer_family};

  const auto suitable{has_required_features && has_device_extensions && has_queue_families
                      && non_repeated_queue_family_indices};

  if (suitable) {
    log_info("Suitable device found: {}", dev_props.deviceName);
    return true;
  } else {
    log_info("{} {} {}", has_required_features, has_device_extensions, has_queue_families);
    return false;
  }
}

auto surge::renderer::vk::create_logical_device(VkPhysicalDevice phys_dev) -> Result<VkDevice> {
  log_info("Creating logical device");

  // Enabled features. We have to restore the pointer chain explicitly.
  auto [features_12, features_13] = get_required_device_features();

  features_12.pNext = &features_13;

  VkPhysicalDeviceFeatures2 features{};
  features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  features.pNext = &features_12;

  // Extensions
  const auto device_extensions{get_required_device_extensions()};

  // Queues
  const auto device_queue_families{get_available_device_queue_families(phys_dev)};
  const auto indices{get_queue_family_indices(device_queue_families)};

  constexpr float queue_priority{1.0f};

  VkDeviceQueueCreateInfo graphics_queue_ci{.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                            .pNext = nullptr,
                                            .flags = 0,
                                            .queueFamilyIndex = indices.graphics_family,
                                            .queueCount = 1,
                                            .pQueuePriorities = &queue_priority};

  VkDeviceQueueCreateInfo transfer_queue_ci{.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                            .pNext = nullptr,
                                            .flags = 0,
                                            .queueFamilyIndex = indices.transfer_family,
                                            .queueCount = 1,
                                            .pQueuePriorities = &queue_priority};

  VkDeviceQueueCreateInfo compute_queue_ci{.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                           .pNext = nullptr,
                                           .flags = 0,
                                           .queueFamilyIndex = indices.compute_family,
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
      .enabledExtensionCount = static_cast<u32>(device_extensions.size()),
      .ppEnabledExtensionNames = device_extensions.data(),
      .pEnabledFeatures = nullptr};

  VkDevice log_dev{};
  const auto result{vkCreateDevice(phys_dev, &create_info, get_alloc_callbacks(), &log_dev)};

  if (result != VK_SUCCESS) {
    log_error("Unable create logical device:");
    return Err{Error::vk_log_dev_create};
  } else {
    return log_dev;
  }
}

auto surge::renderer::vk::create_window_surface(window::Window w, VkInstance instance)
    -> Result<VkSurfaceKHR> {
  log_info("Creating window surface");

  VkSurfaceKHR surface{};
  const auto result{glfwCreateWindowSurface(instance, w, get_alloc_callbacks(), &surface)};

  if (result != VK_SUCCESS) {
    log_error("Window Vulkan surface creation failed:");
    return Err{Error::vk_surface_init};
  } else {
    log_info("Window surface created");
    return surface;
  }
}

auto surge::renderer::vk::get_queue_handles(VkPhysicalDevice phys_dev, VkDevice log_dev,
                                            VkSurfaceKHR surface) -> Result<QueueHandles> {
  log_info("Getting queue handles");

  QueueHandles handles{};

  const auto device_queue_families{get_available_device_queue_families(phys_dev)};
  const auto indices{get_queue_family_indices(device_queue_families)};

  VkBool32 graphics_present_suport{false};
  const auto result{vkGetPhysicalDeviceSurfaceSupportKHR(phys_dev, indices.graphics_family, surface,
                                                         &graphics_present_suport)};
  if (result != VK_SUCCESS) {
    log_error("Unable to query graphics queue for presentation support");
    return Err{Error::vk_surface_present_query};
  }

  if (!graphics_present_suport) {
    log_error("The graphics queue provided by the device does not support presentation");
    return Err{Error::vk_surface_present_unable};
  }

  handles.graphics_idx = indices.graphics_family;
  handles.transfer_idx = indices.transfer_family;
  handles.compute_idx = indices.compute_family;

  vkGetDeviceQueue(log_dev, handles.graphics_idx, 0, &(handles.graphics));
  vkGetDeviceQueue(log_dev, handles.transfer_idx, 0, &(handles.transfer));
  vkGetDeviceQueue(log_dev, handles.compute_idx, 0, &(handles.compute));

  log_info("Queue handles obtained");

  return handles;
}

auto surge::renderer::vk::create_swapchain(VkPhysicalDevice phys_dev, VkDevice log_dev,
                                           VkSurfaceKHR surface,
                                           const config::RendererAttributes &r_attrs, u32 width,
                                           u32 height, VkSwapchainKHR old_swapchain)
    -> Result<SwapchainData> {

  log_info("Creating swapchain");

  using std::clamp;

  SwapchainData swpc_data{};

  // Capabilities
  VkSurfaceCapabilitiesKHR surface_capabilities{};
  auto result{vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys_dev, surface, &surface_capabilities)};

  if (result != VK_SUCCESS) {
    log_error("Unable to query surface capabilities:");
    return Err{Error::vk_swapchain_query};
  }

  // Image extents
  VkExtent2D extent{clamp(width, surface_capabilities.minImageExtent.width,
                          surface_capabilities.maxImageExtent.width),
                    clamp(height, surface_capabilities.minImageExtent.height,
                          surface_capabilities.maxImageExtent.height)};
  swpc_data.extent = extent;

  // Image formats
  const auto img_format{VK_FORMAT_B8G8R8A8_UNORM};
  const auto img_colorspace{VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
  const auto img_usage{VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT};

  // Image count
  uint32_t image_count{surface_capabilities.minImageCount + 1};

  if (surface_capabilities.maxImageCount > 0 && image_count > surface_capabilities.maxImageCount) {
    image_count = surface_capabilities.maxImageCount;
  }

  // Presentation mode
  VkPresentModeKHR present_mode{};

  // VSync
  if (r_attrs.vsync) {
    present_mode = VK_PRESENT_MODE_FIFO_KHR;
  } else {
    present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
  }

  VkSwapchainCreateInfoKHR swpc_ci{};
  swpc_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swpc_ci.pNext = nullptr;
  swpc_ci.flags = 0;
  swpc_ci.surface = surface;
  swpc_ci.minImageCount = image_count;
  swpc_ci.imageFormat = img_format;
  swpc_ci.imageColorSpace = img_colorspace;
  swpc_ci.imageExtent = extent;
  swpc_ci.imageArrayLayers = 1;
  swpc_ci.imageUsage = img_usage;
  swpc_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  swpc_ci.queueFamilyIndexCount = 0;
  swpc_ci.pQueueFamilyIndices = nullptr;
  swpc_ci.preTransform = surface_capabilities.currentTransform;
  swpc_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swpc_ci.presentMode = present_mode;
  swpc_ci.clipped = VK_TRUE;
  swpc_ci.oldSwapchain = old_swapchain;

  result = vkCreateSwapchainKHR(log_dev, &swpc_ci, get_alloc_callbacks(), &swpc_data.swapchain);
  if (result != VK_SUCCESS) {
    log_error("Unable to create swapchain:");
    return Err{Error::vk_init_swapchain};
  }

  result = vkGetSwapchainImagesKHR(log_dev, swpc_data.swapchain, &image_count, nullptr);
  if (result != VK_SUCCESS) {
    log_error("Unable to get swapchain images:");
    return Err{Error::vk_swapchain_imgs};
  }

  swpc_data.imgs.resize(image_count);
  result
      = vkGetSwapchainImagesKHR(log_dev, swpc_data.swapchain, &image_count, swpc_data.imgs.data());
  if (result != VK_SUCCESS) {
    log_error("Unable to get swapchain images:");
    return Err{Error::vk_swapchain_imgs};
  }

  const VkComponentMapping img_view_components{
      VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
      VK_COMPONENT_SWIZZLE_IDENTITY};

  VkImageSubresourceRange img_view_sub_range{};
  img_view_sub_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  img_view_sub_range.baseMipLevel = 0;
  img_view_sub_range.levelCount = 1;
  img_view_sub_range.baseArrayLayer = 0;
  img_view_sub_range.layerCount = 1;

  for (const auto &img : swpc_data.imgs) {
    VkImageViewCreateInfo img_view_ci{};
    img_view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    img_view_ci.pNext = nullptr;
    img_view_ci.flags = 0;
    img_view_ci.image = img;
    img_view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    img_view_ci.format = img_format;
    img_view_ci.components = img_view_components;
    img_view_ci.subresourceRange = img_view_sub_range;

    VkImageView img_view{};
    result = vkCreateImageView(log_dev, &img_view_ci, get_alloc_callbacks(), &img_view);
    if (result != VK_SUCCESS) {
      log_error("Unable to create swapchain image view:");
      return Err{Error::vk_swapchain_imgs_views};
    } else {
      swpc_data.imgs_views.push_back(img_view);
    }
  }

  log_info("Swapchain created");
  return swpc_data;
}

auto surge::renderer::vk::create_frame_data(VkDevice device, u32 graphics_queue_idx)
    -> Result<FrameData> {
  log_info("Creating frame data");

  FrameData frm_data{};

  // Command structures
  auto cmd_pool_create_info{command_pool_create_info(
      graphics_queue_idx, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)};

  for (usize i = 0; i < frm_data.frame_overlap; i++) {
    auto result{vkCreateCommandPool(device, &cmd_pool_create_info, get_alloc_callbacks(),
                                    &frm_data.command_pools[i])}; // NOLINT

    if (result != VK_SUCCESS) {
      log_error("Unable to create command pool:");
      return Err{Error::vk_cmd_pool_creation};
    }

    // Command buffer
    // NOLINTNEXTLINE
    auto cmd_buffer{command_buffer_alloc_info(frm_data.command_pools[i], 1)};
    result = vkAllocateCommandBuffers(device, &cmd_buffer,
                                      &frm_data.command_buffers[i]); // NOLINT

    if (result != VK_SUCCESS) {
      log_error("Unable to create buffer:");
      return Err{Error::vk_cmd_buffer_creation};
    }
  }

  // Synchronization structures
  auto fence_ci{fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT)};
  auto sem_ci{semaphore_create_info()};

  for (usize i = 0; i < frm_data.frame_overlap; i++) {
    auto result{vkCreateFence(device, &fence_ci, get_alloc_callbacks(),
                              &frm_data.render_fences[i])}; // NOLINT

    if (result != VK_SUCCESS) {
      log_error("Unable to create fence:");
      return Err{Error::vk_fence_creation};
    }

    result = vkCreateSemaphore(device, &sem_ci, get_alloc_callbacks(),
                               &frm_data.render_semaphores[i]); // NOLINT

    if (result != VK_SUCCESS) {
      log_error("Unable to create renderer semaphore:");
      return Err{Error::vk_semaphore_creation};
    }

    result = vkCreateSemaphore(device, &sem_ci, get_alloc_callbacks(),
                               &frm_data.swpc_semaphores[i]); // NOLINT

    if (result != VK_SUCCESS) {
      log_error("Unable to create swapchain semaphore:");
      return Err{Error::vk_fence_creation};
    }
  }

  log_info("Frame data created");
  return frm_data;
}

void surge::renderer::vk::destroy_frame_data(VkDevice device, FrameData &frm_data) {
  log_info("Destroying frame data");

  for (usize i = 0; i < frm_data.frame_overlap; i++) {
    // NOLINTNEXTLINE
    vkDestroySemaphore(device, frm_data.swpc_semaphores[i], get_alloc_callbacks());

    // NOLINTNEXTLINE
    vkDestroySemaphore(device, frm_data.render_semaphores[i], get_alloc_callbacks());

    // NOLINTNEXTLINE
    vkDestroyFence(device, frm_data.render_fences[i], get_alloc_callbacks());

    // NOLINTNEXTLINE
    vkFreeCommandBuffers(device, frm_data.command_pools[i], 1,
                         &frm_data.command_buffers[i]); // NOLINT

    // NOLINTNEXTLINE
    vkDestroyCommandPool(device, frm_data.command_pools[i], get_alloc_callbacks());
  }

  log_info("Frame data destroyied");
}

auto surge::renderer::vk::create_immediate_mode_data(VkDevice device, u32 graphics_queue_idx)
    -> Result<ImmediateModeData> {
  log_info("Creating immediate mode command data");

  ImmediateModeData data{};

  // Cmd pool
  auto cmd_pool_create_info{command_pool_create_info(
      graphics_queue_idx, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)};
  auto result{
      vkCreateCommandPool(device, &cmd_pool_create_info, get_alloc_callbacks(), &data.cmd_pool)};

  if (result != VK_SUCCESS) {
    log_error("Unable to allocate immediate mode command pool:");
    return Err{Error::vk_cmd_pool_creation};
  }

  // Cmd buffer
  const auto cmd_buffer_info{command_buffer_alloc_info(data.cmd_pool, 1)};
  result = vkAllocateCommandBuffers(device, &cmd_buffer_info, &data.cmd_buff);

  if (result != VK_SUCCESS) {
    log_error("Unable to allocate immediate mode command buffer:");
    return Err{Error::vk_cmd_buffer_creation};
  }

  // Sync Structures
  auto fence_ci{fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT)};
  result = vkCreateFence(device, &fence_ci, get_alloc_callbacks(), &data.fence);

  if (result != VK_SUCCESS) {
    log_error("Unable to create immediate mode fence:");
    return Err{Error::vk_fence_creation};
  }

  return data;
}

void surge::renderer::vk::destroy_immediate_mode_data(VkDevice device, ImmediateModeData &data) {
  vkDestroyFence(device, data.fence, get_alloc_callbacks());
  vkDestroyCommandPool(device, data.cmd_pool, get_alloc_callbacks());
}