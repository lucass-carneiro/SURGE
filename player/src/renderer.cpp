#include "renderer.hpp"

#include "allocators.hpp"
#include "error_types.hpp"
#include "integer_types.hpp"
#include "logging.hpp"
#include "options.hpp"

#include <cstring>
#include <optional>

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

static auto check_vk_validation_layers(const surge::vector<const char *> &requested_layers) noexcept
    -> bool {
  using namespace surge;
  using std::strcmp;

  u32 layer_count{0};
  vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

  vector<VkLayerProperties> available_layers(layer_count);
  available_layers.reserve(layer_count);

  vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

  log_info("There are %u validation layers available:", layer_count);
  for (const auto &layer : available_layers) {
    log_info("  Validation layer: %s", layer.layerName);
  }

  for (const auto &requested_layer : requested_layers) {
    bool layer_found = false;

    for (const auto &available_layer : available_layers) {
      if (strcmp(requested_layer, available_layer.layerName) == 0) {
        layer_found = true;
        log_info("Enabeling %s", requested_layer);
        break;
      }
    }

    if (!layer_found) {
      return false;
    }
  }

  return true;
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

static auto get_extensions() noexcept -> tl::expected<surge::vector<const char *>, surge::error> {
  using namespace surge;
  u32 glfw_ext_count{0};

  auto glfw_ext_list{glfwGetRequiredInstanceExtensions(&glfw_ext_count)};
  if (glfwGetError(nullptr) != GLFW_NO_ERROR) {
    log_error("  Ubable to query required Vulkan Extensions from GLFW.");
    return tl::unexpected{surge::error::glfw_ext_querry};
  }

  vector<const char *> extensions(glfw_ext_list, glfw_ext_list + glfw_ext_count);

#ifdef SURGE_BUILD_TYPE_Debug
  extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

  return extensions;
}

static auto create_debug_msg(surge::renderer::context &vk_ctx,
                             VkDebugUtilsMessengerCreateInfoEXT &dmci) noexcept
    -> std::optional<surge::error> {
  using namespace surge;

  auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(vk_ctx.instance, "vkCreateDebugUtilsMessengerEXT"));
  if (func != nullptr) {
    if (func(vk_ctx.instance, &dmci, &vk_ctx.alloc_callbacks, &vk_ctx.debug_messager)
        != VK_SUCCESS) {
      log_error("Error while creating Debug Messenger.");
      return error::vk_debug_msg;
    }
  } else {
    log_error("vkCreateDebugUtilsMessengerEXT not present");
    return error::vk_ext_not_found;
  }

  return {};
}

static void destroy_debug_msg(surge::renderer::context &vk_ctx) noexcept {
  auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(vk_ctx.instance, "vkDestroyDebugUtilsMessengerEXT"));
  if (func != nullptr) {
    func(vk_ctx.instance, vk_ctx.debug_messager, &vk_ctx.alloc_callbacks);
  }
}

static auto find_graphics_queue_family(VkPhysicalDevice &physical_device) noexcept
    -> tl::expected<surge::u32, surge::error> {
  using namespace surge;

  std::optional<u32> graphics_family_idx{};

  u32 queue_family_count{0};
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

  vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count,
                                           queue_families.data());

  for (u32 i = 0; const auto &queue_family : queue_families) {
    if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      graphics_family_idx = i;
    }
    i++;
  }

  if (!graphics_family_idx.has_value()) {
    log_error("Unable to find graphics queue family");
    return tl::unexpected{error::vk_no_graphics_queue};
  } else {
    return graphics_family_idx.value();
  }
}

static auto find_present_queue_family(VkPhysicalDevice &physical_device,
                                      VkSurfaceKHR &surface) noexcept
    -> tl::expected<surge::u32, surge::error> {
  using namespace surge;

  std::optional<u32> present_family_idx{};

  u32 queue_family_count{0};
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

  vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count,
                                           queue_families.data());

  for (u32 i = 0; i < queue_families.size(); i++) {
    VkBool32 presentSupport{false};
    vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &presentSupport);
    if (presentSupport) {
      present_family_idx = i;
    }
  }

  if (!present_family_idx.has_value()) {
    log_error("Unable to find presentation queue family");
    return tl::unexpected{error::vk_no_present_queue};
  } else {
    return present_family_idx.value();
  }
}

static auto
device_extension_supported(VkPhysicalDevice &physical_device,
                           const surge::vector<const char *> &device_extensions) noexcept -> bool {
  using namespace surge;

  u32 extension_count{0};
  vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr);

  vector<VkExtensionProperties> available_extensions(extension_count);
  vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count,
                                       available_extensions.data());

  set<string> required_extensions(device_extensions.begin(), device_extensions.end());

  for (const auto &extension : available_extensions) {
    required_extensions.erase(extension.extensionName);
  }

  return required_extensions.empty();
}

static auto device_suitable(VkPhysicalDevice &physical_device, VkSurfaceKHR &surface,
                            const surge::vector<const char *> &device_extensions) noexcept -> bool {
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(physical_device, &properties);

  const std::array<bool, 4> requirements{
      properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
      find_graphics_queue_family(physical_device).has_value(),
      find_present_queue_family(physical_device, surface).has_value(),
      device_extension_supported(physical_device, device_extensions)};

  return std::all_of(requirements.begin(), requirements.end(), [](bool x) { return x; });
}

static auto select_physical_device(surge::renderer::context &vk_ctx,
                                   const surge::vector<const char *> &device_extensions) noexcept
    -> tl::expected<VkPhysicalDevice, surge::error> {
  using namespace surge;

  VkPhysicalDevice physical_device{VK_NULL_HANDLE};

  u32 device_count{0};
  vkEnumeratePhysicalDevices(vk_ctx.instance, &device_count, nullptr);

  if (device_count == 0) {
    log_error("Unable to find Vulkan compatible GPU.");
    return tl::unexpected{error::vk_no_device};
  }

  vector<VkPhysicalDevice> devices(device_count);
  vkEnumeratePhysicalDevices(vk_ctx.instance, &device_count, devices.data());

  log_info("Detected %u Vulkan compatible GPU(s)", device_count);

  for (auto &device : devices) {
    if (device_suitable(device, vk_ctx.surface, device_extensions)) {
      physical_device = device;
      log_info("Found suitable GPU");

      VkPhysicalDeviceProperties properties;
      vkGetPhysicalDeviceProperties(physical_device, &properties);

      log_info("Selected GPU:\n"
               "  ID: %u\n"
               "  Name: %s\n"
               "  API Version: %u\n"
               "  Driver Version: %u\n"
               "  Vendor ID: %u",
               properties.deviceID, properties.deviceName, properties.apiVersion,
               properties.driverVersion, properties.vendorID);

      break;
    }
  }

  vk_ctx.graphics_queue_family_idx = find_graphics_queue_family(physical_device).value_or(0);
  vk_ctx.present_queue_family_idx
      = find_present_queue_family(physical_device, vk_ctx.surface).value_or(0);

  if (physical_device == VK_NULL_HANDLE) {
    log_error("Unable to detect suitable GPU");
    return tl::unexpected{error::vk_no_device};
  } else {
    return physical_device;
  }
}

static auto create_logical_device(VkPhysicalDevice &physical_device,
                                  const surge::vector<const char *> &requested_layers,
                                  surge::renderer::context &vk_ctx,
                                  const surge::vector<const char *> &device_extensions) noexcept
    -> std::optional<surge::error> {
  using namespace surge;

  const float queue_priority{1.0f};

  vector<VkDeviceQueueCreateInfo> queue_create_infos{};
  set<u32> unique_queue_families
      = {vk_ctx.graphics_queue_family_idx, vk_ctx.present_queue_family_idx};

  for (auto &queue_family : unique_queue_families) {
    VkDeviceQueueCreateInfo queue_create_info{};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = queue_family;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;
    queue_create_infos.push_back(queue_create_info);
  }

  VkPhysicalDeviceFeatures df{};

  VkDeviceCreateInfo dci{};
  dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  dci.queueCreateInfoCount = static_cast<u32>(queue_create_infos.size());
  dci.pQueueCreateInfos = queue_create_infos.data();
  dci.pEnabledFeatures = &df;
  dci.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
  dci.ppEnabledExtensionNames = device_extensions.data();

#ifdef SURGE_BUILD_TYPE_Debug
  dci.enabledLayerCount = static_cast<uint32_t>(requested_layers.size());
  dci.ppEnabledLayerNames = requested_layers.data();
#else
  dci.enabledLayerCount = 0;
#endif

  if (vkCreateDevice(physical_device, &dci, &vk_ctx.alloc_callbacks, &vk_ctx.device)
      != VK_SUCCESS) {
    log_error("Unable to initialize Vulkan logical device from physical device");
    return error::vk_logical_device;
  }

  return {};
}

auto surge::renderer::init(const string &window_name, GLFWwindow *window) noexcept
    -> tl::expected<context, error> {
  log_info("Initializing Vulkan");

  context vk_ctx{};

  /**************
   * Extensions *
   **************/
  auto extensions{get_extensions()};
  if (!extensions) {
    return tl::unexpected{extensions.error()};
  }

  /************
   * App info *
   ************/
  VkApplicationInfo app_info{};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = window_name.c_str();
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pEngineName = "SURGE - The Super Underrated Game Engine";
  app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.apiVersion = VK_API_VERSION_1_3;

  /***************
   * Create Info *
   ***************/
  VkInstanceCreateInfo ici{};
  ici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  ici.pApplicationInfo = &app_info;
  ici.enabledExtensionCount = static_cast<surge::u32>(extensions->size());
  ici.ppEnabledExtensionNames = extensions->data();

  /*************************
   * Debug MSG create info *
   *************************/
  VkDebugUtilsMessengerCreateInfoEXT dmci{};
  dmci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  dmci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  dmci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                     | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                     | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  dmci.pfnUserCallback = debug_callback;
  dmci.pUserData = nullptr;

  /**************
   * Allocators *
   **************/
  vk_ctx.alloc_callbacks
      = {nullptr, vk_alloc, vk_realloc, vk_free, vk_internal_alloc, vk_internal_free};

/*********************
 * Validation Layers *
 *********************/
#ifdef SURGE_BUILD_TYPE_Debug
  const vector<const char *> requested_layers = {"VK_LAYER_KHRONOS_validation"};

  if (!check_vk_validation_layers(requested_layers)) {
    log_error("Requested validation layers not available");
    return tl::unexpected{error::vk_validation_layers_not_available};
  }

  ici.enabledLayerCount = static_cast<surge::u32>(requested_layers.size());
  ici.ppEnabledLayerNames = requested_layers.data();
  ici.pNext = static_cast<VkDebugUtilsMessengerCreateInfoEXT *>(&dmci);
#else
  ici.enabledLayerCount = 0;
#endif

  /************
   * Instance *
   ************/
  if (vkCreateInstance(&ici, &vk_ctx.alloc_callbacks, &vk_ctx.instance) != VK_SUCCESS) {
    log_error("Error while creating vulkan instance");
    return tl::unexpected{error::vk_instance_create};
  }

  /******************
   * Debug Messager *
   ******************/
  auto create_debug_msg_result{create_debug_msg(vk_ctx, dmci)};
  if (create_debug_msg_result.has_value()) {
    return tl::unexpected{create_debug_msg_result.value()};
  }

  /**********
   * Surface *
   **********/
  if (glfwCreateWindowSurface(vk_ctx.instance, window, &vk_ctx.alloc_callbacks, &vk_ctx.surface)
      != VK_SUCCESS) {
    log_error("Unable to create Vulkan window surface");
    return tl::unexpected{error::vk_surface};
  }

  /**********
   * Device *
   **********/
  const vector<const char *> device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  auto physical_device{select_physical_device(vk_ctx, device_extensions)};
  if (!physical_device) {
    return tl::unexpected{physical_device.error()};
  }

  auto logical_device_result{
      create_logical_device(*physical_device, requested_layers, vk_ctx, device_extensions)};

  if (logical_device_result.has_value()) {
    return tl::unexpected{logical_device_result.value()};
  }

  /*****************
   * Queue handles *
   *****************/
  vkGetDeviceQueue(vk_ctx.device, vk_ctx.graphics_queue_family_idx, 0, &vk_ctx.graphics_queue);
  vkGetDeviceQueue(vk_ctx.device, vk_ctx.present_queue_family_idx, 0, &vk_ctx.present_queue);

  log_info("Vulkan instance initialized.");

  return vk_ctx;
}

void surge::renderer::terminate(context &vk_ctx) noexcept {
  log_info("Terminating Vulkan");

  vkDestroyDevice(vk_ctx.device, &vk_ctx.alloc_callbacks);
  vkDestroySurfaceKHR(vk_ctx.instance, vk_ctx.surface, &vk_ctx.alloc_callbacks);
  destroy_debug_msg(vk_ctx);
  vkDestroyInstance(vk_ctx.instance, &vk_ctx.alloc_callbacks);
}