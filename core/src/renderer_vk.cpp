#define VMA_IMPLEMENTATION

#include "renderer_vk.hpp"

#include "allocators.hpp"
#include "container_types.hpp"
#include "glfw_includes.hpp"
#include "logging.hpp"
#include "window.hpp"

#include <algorithm>
#include <vulkan/vk_enum_string_helper.h>

using namespace surge::renderer;

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
static const VkAllocationCallbacks alloc_callbacks{.pUserData = nullptr,
                                                   .pfnAllocation = vk_malloc,
                                                   .pfnReallocation = vk_realloc,
                                                   .pfnFree = vk_free,
                                                   .pfnInternalAllocation = vk_internal_malloc,
                                                   .pfnInternalFree = vk_internal_free};

#ifdef SURGE_USE_VK_VALIDATION_LAYERS
static VKAPI_ATTR auto VKAPI_CALL debug_callback(
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

auto vk::init_helpers::get_required_extensions() noexcept
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
auto vk::init_helpers::get_required_validation_layers() noexcept
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

auto vk::infos::dbg_msg_create_info() noexcept -> VkDebugUtilsMessengerCreateInfoEXT {
  VkDebugUtilsMessengerCreateInfoEXT create_info{
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .pNext = nullptr,
      .flags = 0,
      .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                     | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                     | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      .pfnUserCallback = debug_callback,
      .pUserData = nullptr};

  return create_info;
}

auto vk::init_helpers::create_dbg_msg(VkInstance instance) noexcept
    -> tl::expected<VkDebugUtilsMessengerEXT, error> {
  using namespace infos;

  log_info("Creating debug messenger");

  auto func{reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"))};

  if (func == nullptr) {
    log_error("Unable to find extension function vkCreateDebugUtilsMessengerEXT");
    return tl::unexpected{error::vk_dbg_msg_ext_func_ptr};
  }

  auto create_info{dbg_msg_create_info()};

  VkDebugUtilsMessengerEXT dbg_msg{};
  const auto result{func(instance, &create_info, &alloc_callbacks, &dbg_msg)};

  if (result != VK_SUCCESS) {
    log_error("Unable create debug messenger: {}", string_VkResult(result));
    return tl::unexpected{error::vk_dbg_msg_create};
  } else {
    return dbg_msg;
  }
}

auto vk::init_helpers::destroy_dbg_msg(VkInstance instance, VkDebugUtilsMessengerEXT dbg_msg)
    -> tl::expected<void, error> {
  log_info("Destroying debug messenger");

  auto func{reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"))};

  if (func == nullptr) {
    log_error("Unable to find extension function vkDestroyDebugUtilsMessengerEXT");
    return tl::unexpected{error::vk_dbg_msg_ext_func_ptr};
  }

  func(instance, dbg_msg, &alloc_callbacks);

  return {};
}

#endif

auto vk::init_helpers::create_instance() noexcept -> tl::expected<VkInstance, error> {
  using namespace infos;

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
  const auto result{vkCreateInstance(&create_info, &alloc_callbacks, &instance)};
  if (result != VK_SUCCESS) {
    log_error("Unable initialize vulkan instance: {}", string_VkResult(result));
    return tl::unexpected{error::vk_instance_init};
  }

  return instance;
}

auto vk::init_helpers::find_queue_families(VkPhysicalDevice phys_dev) noexcept
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

auto vk::init_helpers::get_required_device_extensions(VkPhysicalDevice phys_dev) noexcept
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

auto vk::init_helpers::is_device_suitable(VkPhysicalDevice phys_dev) noexcept -> bool {
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

auto vk::init_helpers::select_physical_device(VkInstance instance) noexcept
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

auto vk::init_helpers::create_logical_device(VkPhysicalDevice phys_dev) noexcept
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
  const auto result{vkCreateDevice(phys_dev, &create_info, &alloc_callbacks, &log_dev)};

  if (result != VK_SUCCESS) {
    log_error("Unable create logical device: {}", string_VkResult(result));
    return tl::unexpected{error::vk_log_dev_create};
  } else {
    return log_dev;
  }
}

auto vk::init_helpers::create_window_surface(VkInstance instance) noexcept
    -> tl::expected<VkSurfaceKHR, error> {
  log_info("Creating window surface");

  VkSurfaceKHR surface{};
  const auto result{
      glfwCreateWindowSurface(instance, window::get_window_ptr(), &alloc_callbacks, &surface)};

  if (result != VK_SUCCESS) {
    log_error("Window Vulkan surface creation failed: {}", string_VkResult(result));
    return tl::unexpected{error::vk_surface_init};
  } else {
    return surface;
  }
}

auto vk::init_helpers::get_queue_handles(VkPhysicalDevice phys_dev, VkDevice log_dev,
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

auto vk::init_helpers::create_swapchain(VkPhysicalDevice phys_dev, VkDevice log_dev,
                                        VkSurfaceKHR surface, const config::renderer_attrs &r_attrs,
                                        u32 width, u32 height) noexcept
    -> tl::expected<swapchain_data, error> {
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

  result = vkCreateSwapchainKHR(log_dev, &swpc_create_info, &alloc_callbacks, &swpc_data.swapchain);
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
    result = vkCreateImageView(log_dev, &img_view_ci, &alloc_callbacks, &img_view);
    if (result != VK_SUCCESS) {
      log_error("Unable to create swapchain image view: {}", string_VkResult(result));
      return tl::unexpected{error::vk_swapchain_imgs_views};
    } else {
      swpc_data.imgs_views.push_back(img_view);
    }
  }

  return swpc_data;
}

auto vk::infos::command_pool_create_info(
    u32 queue_family_idx, VkCommandPoolCreateFlags flags) noexcept -> VkCommandPoolCreateInfo {
  VkCommandPoolCreateInfo ci{.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                             .pNext = nullptr,
                             .flags = flags,
                             .queueFamilyIndex = queue_family_idx};
  return ci;
}

auto vk::infos::command_buffer_alloc_info(VkCommandPool pool,
                                          u32 count) noexcept -> VkCommandBufferAllocateInfo {
  VkCommandBufferAllocateInfo ai{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                 .pNext = nullptr,
                                 .commandPool = pool,
                                 .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                 .commandBufferCount = count};
  return ai;
}

auto vk::infos::command_buffer_begin_info(VkCommandBufferUsageFlags flags) noexcept
    -> VkCommandBufferBeginInfo {
  VkCommandBufferBeginInfo ci = {};
  ci.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  ci.pNext = nullptr;
  ci.pInheritanceInfo = nullptr;
  ci.flags = flags;
  return ci;
}

auto vk::infos::command_buffer_submit_info(VkCommandBuffer cmd) noexcept
    -> VkCommandBufferSubmitInfo {
  VkCommandBufferSubmitInfo si{};
  si.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
  si.pNext = nullptr;
  si.commandBuffer = cmd;
  si.deviceMask = 0;
  return si;
}

auto vk::infos::semaphore_submit_info(VkPipelineStageFlags2 stage_mask,
                                      VkSemaphore semaphore) noexcept -> VkSemaphoreSubmitInfo {
  VkSemaphoreSubmitInfo si{};
  si.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
  si.pNext = nullptr;
  si.semaphore = semaphore;
  si.stageMask = stage_mask;
  si.deviceIndex = 0;
  si.value = 1;
  return si;
}

auto vk::infos::submit_info(VkCommandBufferSubmitInfo *cmd, VkSemaphoreSubmitInfo *signam_sem_info,
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

auto vk::infos::fence_create_info(VkFenceCreateFlags flags) noexcept -> VkFenceCreateInfo {
  VkFenceCreateInfo ci{};
  ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  ci.pNext = nullptr;
  ci.flags = flags;
  return ci;
}

auto vk::infos::semaphore_create_info(VkSemaphoreCreateFlags flags) noexcept
    -> VkSemaphoreCreateInfo {
  VkSemaphoreCreateInfo ci{};
  ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  ci.pNext = nullptr;
  ci.flags = flags;
  return ci;
}

auto vk::init_helpers::create_frame_data(VkDevice device, u32 graphics_queue_idx) noexcept
    -> tl::expected<frame_data, error> {
  using namespace infos;

  frame_data frm_data{};

  /******************************
   * Command structures *
   ******************************/
  auto cmd_pool_create_info{command_pool_create_info(
      graphics_queue_idx, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)};

  for (usize i = 0; i < frm_data.frame_overlap; i++) {
    auto result{vkCreateCommandPool(device, &cmd_pool_create_info, &alloc_callbacks,
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
    auto result{
        vkCreateFence(device, &fence_ci, &alloc_callbacks, &frm_data.render_fences[i])}; // NOLINT

    if (result != VK_SUCCESS) {
      log_error("Unable to create fence: {}", string_VkResult(result));
      return tl::unexpected{error::vk_fence_creation};
    }

    result = vkCreateSemaphore(device, &sem_ci, &alloc_callbacks,
                               &frm_data.render_semaphores[i]); // NOLINT

    if (result != VK_SUCCESS) {
      log_error("Unable to create renderer semaphore: {}", string_VkResult(result));
      return tl::unexpected{error::vk_semaphore_creation};
    }

    result = vkCreateSemaphore(device, &sem_ci, &alloc_callbacks,
                               &frm_data.swpc_semaphores[i]); // NOLINT

    if (result != VK_SUCCESS) {
      log_error("Unable to create swapchain semaphore: {}", string_VkResult(result));
      return tl::unexpected{error::vk_fence_creation};
    }
  }

  return frm_data;
}

void vk::init_helpers::destroy_frame_data(context &ctx) noexcept {
  log_info("Destroying frame data");
  for (usize i = 0; i < ctx.frm_data.frame_overlap; i++) {
    // NOLINTNEXTLINE
    vkDestroySemaphore(ctx.log_dev, ctx.frm_data.swpc_semaphores[i], &alloc_callbacks);

    // NOLINTNEXTLINE
    vkDestroySemaphore(ctx.log_dev, ctx.frm_data.render_semaphores[i], &alloc_callbacks);

    // NOLINTNEXTLINE
    vkDestroyFence(ctx.log_dev, ctx.frm_data.render_fences[i], &alloc_callbacks);

    // NOLINTNEXTLINE
    vkFreeCommandBuffers(ctx.log_dev, ctx.frm_data.command_pools[i], 1,
                         &ctx.frm_data.command_buffers[i]); // NOLINT

    // NOLINTNEXTLINE
    vkDestroyCommandPool(ctx.log_dev, ctx.frm_data.command_pools[i], &alloc_callbacks);
  }
}

auto vk::init_helpers::create_memory_allocator(VkInstance instance, VkPhysicalDevice phys_dev,
                                               VkDevice logi_dev) noexcept
    -> tl::expected<VmaAllocator, error> {
  log_info("Creating memory allocator");

  VmaAllocatorCreateInfo alloc_info{};
  alloc_info.physicalDevice = phys_dev;
  alloc_info.device = logi_dev;
  alloc_info.instance = instance;
  alloc_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

  VmaAllocator allocator{};
  const auto result{vmaCreateAllocator(&alloc_info, &allocator)};

  if (result != VK_SUCCESS) {
    log_error("Unable create memory allocator: {}", string_VkResult(result));
    return tl::unexpected{error::vk_allocator_creation};
  }

  return allocator;
}

auto vk::init_helpers::image_create_info(VkFormat format, VkImageUsageFlags usage_flags,
                                         VkExtent3D extent) noexcept -> VkImageCreateInfo {
  VkImageCreateInfo ci{};
  ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  ci.pNext = nullptr;

  ci.imageType = VK_IMAGE_TYPE_2D;

  ci.format = format;
  ci.extent = extent;

  ci.mipLevels = 1;
  ci.arrayLayers = 1;

  // TODO: Enable MSAA when requested.
  ci.samples = VK_SAMPLE_COUNT_1_BIT;

  ci.tiling = VK_IMAGE_TILING_OPTIMAL;
  ci.usage = usage_flags;
  return ci;
}

auto vk::infos::imageview_create_info(VkFormat format, VkImage image,
                                      VkImageAspectFlags aspect_flags) noexcept
    -> VkImageViewCreateInfo {
  VkImageViewCreateInfo ci{};
  ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  ci.pNext = nullptr;

  ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
  ci.image = image;
  ci.format = format;
  ci.subresourceRange.baseMipLevel = 0;
  ci.subresourceRange.levelCount = 1;
  ci.subresourceRange.baseArrayLayer = 0;
  ci.subresourceRange.layerCount = 1;
  ci.subresourceRange.aspectMask = aspect_flags;
  return ci;
}

auto vk::init_helpers::image_subresource_range(VkImageAspectFlags aspect_mask) noexcept
    -> VkImageSubresourceRange {
  VkImageSubresourceRange sum_img{};
  sum_img.aspectMask = aspect_mask;
  sum_img.baseMipLevel = 0;
  sum_img.levelCount = VK_REMAINING_MIP_LEVELS;
  sum_img.baseArrayLayer = 0;
  sum_img.layerCount = VK_REMAINING_ARRAY_LAYERS;
  return sum_img;
}

void vk::init_helpers::transition_image(VkCommandBuffer cmd, VkImage image,
                                        VkImageLayout curr_layout,
                                        VkImageLayout new_layout) noexcept {
  // See https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples for finer sync
  // options

  VkImageAspectFlags aspect_mask((new_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
                                     ? VK_IMAGE_ASPECT_DEPTH_BIT
                                     : VK_IMAGE_ASPECT_COLOR_BIT);

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

void vk::init_helpers::image_blit(VkCommandBuffer cmd, VkImage source, VkImage destination,
                                  VkExtent2D src_size, VkExtent2D dst_size) noexcept {
  VkImageBlit2 blit_reg{};
  blit_reg.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
  blit_reg.pNext = nullptr;

  blit_reg.srcOffsets[1].x = static_cast<i32>(src_size.width);
  blit_reg.srcOffsets[1].y = static_cast<i32>(src_size.height);
  blit_reg.srcOffsets[1].z = 1;

  blit_reg.dstOffsets[1].x = static_cast<i32>(dst_size.width);
  blit_reg.dstOffsets[1].y = static_cast<i32>(dst_size.height);
  blit_reg.dstOffsets[1].z = 1;

  blit_reg.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blit_reg.srcSubresource.baseArrayLayer = 0;
  blit_reg.srcSubresource.layerCount = 1;
  blit_reg.srcSubresource.mipLevel = 0;

  blit_reg.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  blit_reg.dstSubresource.baseArrayLayer = 0;
  blit_reg.dstSubresource.layerCount = 1;
  blit_reg.dstSubresource.mipLevel = 0;

  VkBlitImageInfo2 blitInfo{};
  blitInfo.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
  blitInfo.pNext = nullptr;
  blitInfo.dstImage = destination;
  blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  blitInfo.srcImage = source;
  blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  blitInfo.filter = VK_FILTER_LINEAR;
  blitInfo.regionCount = 1;
  blitInfo.pRegions = &blit_reg;

  vkCmdBlitImage2(cmd, &blitInfo);
}

auto vk::init_helpers::create_draw_img(const config::window_resolution &w_res, VkDevice logi_dev,
                                       VmaAllocator allocator) noexcept
    -> tl::expected<allocated_image, error> {
  using namespace infos;

  log_info("Creating draw image target");

  allocated_image draw_image{};

  VkExtent3D draw_image_extent{static_cast<u32>(w_res.width), static_cast<u32>(w_res.height), 1};
  draw_image.image_format = VK_FORMAT_R16G16B16A16_SFLOAT;
  draw_image.image_extent = draw_image_extent;

  VkImageUsageFlags draw_image_usage_flags{};
  draw_image_usage_flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  draw_image_usage_flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  draw_image_usage_flags |= VK_IMAGE_USAGE_STORAGE_BIT;
  draw_image_usage_flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  auto rimg_info{
      image_create_info(draw_image.image_format, draw_image_usage_flags, draw_image_extent)};

  VmaAllocationCreateInfo rimg_allocinfo{};
  rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  // Allocate and create the image
  auto result{vmaCreateImage(allocator, &rimg_info, &rimg_allocinfo, &draw_image.image,
                             &draw_image.allocation, nullptr)};
  if (result != VK_SUCCESS) {
    log_error("Unable to create draw image: {}", string_VkResult(result));
    return tl::unexpected{error::vk_init_draw_img};
  }

  // Build a image-view for the draw image to use for rendering
  auto rview_info{
      imageview_create_info(draw_image.image_format, draw_image.image, VK_IMAGE_ASPECT_COLOR_BIT)};

  result = vkCreateImageView(logi_dev, &rview_info, &alloc_callbacks, &draw_image.image_view);

  if (result != VK_SUCCESS) {
    log_error("Unable to create draw image view: {}", string_VkResult(result));
    return tl::unexpected{error::vk_init_draw_img};
  }

  return draw_image;
}

auto vk::clear(context &ctx, const config::clear_color &w_ccl) noexcept -> std::optional<error> {
  using namespace init_helpers;
  using namespace infos;

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

auto vk::init(const config::renderer_attrs &r_attrs, const config::window_resolution &w_res,
              const config::window_attrs &) noexcept -> tl::expected<context, error> {
  using namespace init_helpers;

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

void vk::terminate(context &ctx) noexcept {
  log_info("Terminating vulkan");

  log_info("Waiting for GPU idle");
  vkWaitForFences(ctx.log_dev, ctx.frm_data.frame_overlap, ctx.frm_data.render_fences.data(), true,
                  1000000000);

  log_info("Destroying draw image");
  vkDestroyImageView(ctx.log_dev, ctx.draw_image.image_view, &alloc_callbacks);
  vmaDestroyImage(ctx.allocator, ctx.draw_image.image, ctx.draw_image.allocation);

  log_info("Destroying memory allocator");
  vmaDestroyAllocator(ctx.allocator);

  init_helpers::destroy_frame_data(ctx);

  log_info("Destroying image views");
  for (const auto &img_view : ctx.swpc_data.imgs_views) {
    vkDestroyImageView(ctx.log_dev, img_view, &alloc_callbacks);
  }

  log_info("Destroying swapchain");
  vkDestroySwapchainKHR(ctx.log_dev, ctx.swpc_data.swapchain, &alloc_callbacks);

  log_info("Destroying window surface");
  vkDestroySurfaceKHR(ctx.instance, ctx.surface, &alloc_callbacks);

  log_info("Destroying logical device");
  vkDestroyDevice(ctx.log_dev, &alloc_callbacks);

  init_helpers::destroy_dbg_msg(ctx.instance, ctx.dbg_msg);

  log_info("Destroying instance");
  vkDestroyInstance(ctx.instance, &alloc_callbacks);
}