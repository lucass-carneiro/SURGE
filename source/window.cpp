#include "window.hpp"
#include "log.hpp"
#include "options.hpp"

//clang-format off
#include <EASTL/set.h>
#include <GLFW/glfw3.h>
//clang-format on

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <tl/expected.hpp>

void surge::glfw_error_callback(int code, const char *description) noexcept {
  log_all<log_event::error>("GLFW error code {}: {}", code, description);
}

auto surge::querry_available_monitors() noexcept
    -> std::optional<std::pair<GLFWmonitor **, std::size_t>> {
  using tl::unexpected;

  int count = 0;
  GLFWmonitor **monitors = glfwGetMonitors(&count);

  if (monitors == nullptr) {
    return {};
  }

  log_all<log_event::message>("Monitors detected: {}", count);

  for (int i = 0; i < count; i++) {
    int width = 0, height = 0;
    float xscale = 0, yscale = 0;
    int xpos = 0, ypos = 0;
    int w_xpos = 0, w_ypos = 0, w_width = 0, w_height = 0;

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    glfwGetMonitorPhysicalSize(monitors[i], &width, &height);

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    glfwGetMonitorContentScale(monitors[i], &xscale, &yscale);

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    glfwGetMonitorPos(monitors[i], &xpos, &ypos);

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    glfwGetMonitorWorkarea(monitors[i], &w_xpos, &w_ypos, &w_width, &w_height);

    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const char *name = glfwGetMonitorName(monitors[i]);
    if (name == nullptr) {
      return {};
    }

    // clang-format off
    log_all<log_event::message>(
        "Properties of monitor {}:\n"
        "  Monitor name: {}.\n"
        "  Physical size (width, height): {}, {}.\n"
        "  Content scale (x, y): {}, {}.\n"
        "  Virtual position: (x, y): {}, {}.\n"
        "  Work area (x, y, width, height): {}, {}, {}, {}.",
        i,
        name,
        width,
        height,
        xscale,
        yscale,
        xpos,
        ypos,
        w_xpos,
        w_ypos,
        w_width,
        w_height
    );
    // clang-format on
  }

  return std::make_pair(monitors, count);
}

surge::global_vulkan_instance::global_vulkan_instance() noexcept
    : instance_created{false}, logical_device_created{false},
      surface_created{false}, app_info{}, create_info{} {

  log_all<log_event::message>("Initializing Vulkan application info");

  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = application_name;
  app_info.pEngineName = "SURGE";
  app_info.engineVersion = VK_MAKE_VERSION(
      SURGE_VERSION_MAJOR, SURGE_VERSION_MINOR, SURGE_VERSION_PATCH);
  app_info.apiVersion = VK_API_VERSION_1_3;

  log_all<log_event::message>("Initializing Vulkan creation info");
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &app_info;

#ifndef SURGE_VULKAN_VALIDATION
  create_info.enabledLayerCount = 0;
#endif
}

surge::global_vulkan_instance::~global_vulkan_instance() noexcept {
  // TODO: Replace nullptr with allocator callbacks
  log_all<log_event::message>("Destroying Vulkan objects");

  if (logical_device_created) {
    vkDestroyDevice(logical_device, nullptr);
  }

  if (surface_created) {
    vkDestroySurfaceKHR(instance, window_surface, nullptr);
  }

  if (instance_created) {
    vkDestroyInstance(instance, nullptr);
  }
}

auto surge::global_vulkan_instance::create_instance() noexcept -> bool {
  log_all<log_event::message>("Initializing Vulkan instance");

  // TODO: Replace nullptr with allocator callbacks
  const auto result = vkCreateInstance(&create_info, nullptr, &instance);

  if (result != VK_SUCCESS) {
    log_all<log_event::error>("Error creating Vulkan instance. Error code {}",
                              result);
  } else {
    instance_created = true;
    log_all<log_event::message>("Vulkan instance created");
  }

  return instance_created;
}

void surge::global_vulkan_instance::get_required_extensions() noexcept {
  log_all<log_event::message>("Querying required Vulkan extensions");

  std::uint32_t glfw_required_extension_count = 0;
  auto glfw_required_extensions =
      glfwGetRequiredInstanceExtensions(&glfw_required_extension_count);

#ifdef SURGE_VULKAN_VALIDATION
  eastl::vector<const char *> required_extensions_tmp(
      glfw_required_extension_count + 1);
#else
  eastl::vector<const char *> required_extensions_tmp(
      glfw_required_extension_count);
#endif

  for (std::uint32_t i = 0; i < glfw_required_extension_count; i++) {
    required_extensions_tmp[i] = glfw_required_extensions[i];
  }

#ifdef SURGE_VULKAN_VALIDATION
  required_extensions_tmp[glfw_required_extension_count] =
      VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
#endif

  required_extensions = std::move(required_extensions_tmp);
}

void surge::global_vulkan_instance::get_available_extensions() noexcept {
  log_all<log_event::message>("Querrying available Vulkan extensions");

  std::uint32_t available_extension_count{0};
  vkEnumerateInstanceExtensionProperties(nullptr, &available_extension_count,
                                         nullptr);

  eastl::vector<VkExtensionProperties> available_extensions_tmp(
      available_extension_count);
  vkEnumerateInstanceExtensionProperties(nullptr, &available_extension_count,
                                         available_extensions_tmp.data());

  available_extensions = std::move(available_extensions_tmp);
}

#ifdef SURGE_VULKAN_VALIDATION
void surge::global_vulkan_instance::get_available_validation_layers() noexcept {
  log_all<log_event::message>("Querrying available Vulkan validation layers");

  std::uint32_t available_layer_count{0};
  vkEnumerateInstanceLayerProperties(&available_layer_count, nullptr);

  eastl::vector<VkLayerProperties> available_layers_tmp(available_layer_count);
  vkEnumerateInstanceLayerProperties(&available_layer_count,
                                     available_layers_tmp.data());

  available_layers = std::move(available_layers_tmp);
}
#endif

auto surge::global_vulkan_instance::check_extensions() noexcept -> bool {
  get_required_extensions();
  get_available_extensions();

  log_all<log_event::message>(
      "Cheking if required Vulkan extensions are available");

  bool extension_found = false;

  for (std::size_t i = 0; i < required_extensions.size(); i++) {
    for (std::size_t j = 0; j < available_extensions.size(); j++) {
      if (std::strcmp(required_extensions[i],
                      available_extensions[j].extensionName) == 0) {
        extension_found = true;
        break;
      }
    }

    if (!extension_found) {
      log_all<log_event::error>("Not all required Vulkan extensions are "
                                "available. First missing extension: {}",
                                required_extensions[i]);
      return false;
    }
  }

  log_all<log_event::message>("All required Vulkan extension(s) were "
                              "found. Enabling {} extension(s).",
                              required_extensions.size());

  create_info.enabledExtensionCount =
      static_cast<std::uint32_t>(required_extensions.size());
  create_info.ppEnabledExtensionNames = required_extensions.data();

  return true;
}

#ifdef SURGE_VULKAN_VALIDATION
auto surge::global_vulkan_instance::check_validation_layers() noexcept -> bool {
  get_available_validation_layers();

  if (required_vulkan_validation_layers.size() > available_layers.size()) {
    log_all<log_event::error>("There are {} available vulkan validation "
                              "layer(s) but {} layer(s) is(are) required",
                              available_layers.size(),
                              required_vulkan_validation_layers.size());
    return false;
  }

  log_all<log_event::message>(
      "Cheking if required validation layers are available");

  bool layer_found = false;

  for (const char *layer_name : required_vulkan_validation_layers) {
    for (const auto &layer_property : available_layers) {
      if (std::strcmp(layer_name, layer_property.layerName) == 0) {
        layer_found = true;
        break;
      }
    }

    if (!layer_found) {
      log_all<log_event::error>("Not all required Vulkan validation layers are "
                                "available. First missing layer: {}",
                                layer_name);
      return false;
    }
  }

  log_all<log_event::message>("All required Vulkan validation layer(s) were "
                              "found. Enabling {} layer(s).",
                              required_vulkan_validation_layers.size());

  create_info.enabledLayerCount = required_vulkan_validation_layers.size();
  create_info.ppEnabledLayerNames = required_vulkan_validation_layers.data();

  return true;
}
#endif

void surge::global_vulkan_instance::get_available_physical_devices() noexcept {
  log_all<log_event::message>(
      "Querying available Vulkan physical devices (GPUs)");

  std::uint32_t device_count{0};
  vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

  eastl::vector<VkPhysicalDevice> available_physical_devices_tmp(device_count);
  vkEnumeratePhysicalDevices(instance, &device_count,
                             available_physical_devices_tmp.data());

  available_physical_devices = std::move(available_physical_devices_tmp);
}

auto surge::global_vulkan_instance::device_type_string(
    std::uint8_t id) const noexcept -> const char * {
  switch (id) {
  case 0:
    return "other";
  case 1:
    return "integrated GPU";
  case 2:
    return "discrete GPU";
  case 3:
    return "virtual GPU";
  case 4:
    return "CPU";
  default:
    return "unknow";
  }
}

auto device_type_string(std::uint8_t id) -> const char *;

void surge::global_vulkan_instance::print_device_summary(
    VkPhysicalDevice device) noexcept {

  VkPhysicalDeviceProperties device_properties;
  vkGetPhysicalDeviceProperties(device, &device_properties);

  log_all<log_event::message>(
      "Selected GPU properties summary:\n"
      "  Name: {}\n"
      "  Type: {} ({})\n"
      "  ID: {}\n"
      "  Vendor ID: {}\n"
      "  API version: {}\n"
      "  Driver version: {}",
      device_properties.deviceName, device_properties.deviceType,
      device_type_string(
          static_cast<std::uint8_t>(device_properties.deviceType)),
      device_properties.deviceID, device_properties.vendorID,
      device_properties.apiVersion, device_properties.driverVersion);
}

auto surge::global_vulkan_instance::is_suitable(
    VkPhysicalDevice device) noexcept -> bool {

  const auto indices = find_queue_families(device);

  return indices.is_complete();
}

auto surge::global_vulkan_instance::pick_physical_device() noexcept -> bool {
  get_available_physical_devices();

  if (available_physical_devices.size() == 0) {
    log_all<log_event::error>(
        "Unable to detect a Vulkan capable GPU in the system.");
    return false;
  }

  log_all<log_event::message>("Cheking GPUs suitabilities");
  for (const auto &device : available_physical_devices) {
    if (is_suitable(device)) {
      selected_physical_device = device;
      break;
    }
  }

  if (selected_physical_device == VK_NULL_HANDLE) {
    log_all<log_event::error>("No suitable GPU found");
    return false;
  }

  log_all<log_event::message>("Suitable GPU found.");
  print_device_summary(selected_physical_device);

  return true;
}

auto surge::global_vulkan_instance::find_queue_families(
    VkPhysicalDevice device) noexcept -> queue_family_indices {

  log_all<log_event::message>("Querying available Vulkan queue families");

  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                                           nullptr);

  eastl::vector<VkQueueFamilyProperties> available_queue_families(
      queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                                           available_queue_families.data());

  log_all<log_event::message>("{} Vulkan queue families found",
                              available_queue_families.size());

  log_all<log_event::message>(
      "Searching for Vulkan queue families with the required capabilities.");

  queue_family_indices indices;

  std::uint32_t i{0};
  for (const auto &family : available_queue_families) {
    if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      log_all<log_event::message>("VK_QUEUE_GRAPHICS_BIT queue found");
      indices.graphics_family = i;
    }

    VkBool32 present_support{false};
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, window_surface,
                                         &present_support);
    if (present_support) {
      indices.present_family = i;
    }

    if (indices.is_complete()) {
      break;
    }

    i++;
  }

  return indices;
}

auto surge::global_vulkan_instance::create_logical_device() noexcept -> bool {
  log_all<log_event::message>("Creating Vulkan queue families");
  const auto indices = find_queue_families(selected_physical_device);

  eastl::vector<VkDeviceQueueCreateInfo> queue_create_infos;
  eastl::set<std::uint32_t> unique_queue_families = {
      indices.graphics_family.value(), indices.present_family.value()};

  float queue_priority{1};
  for (std::uint32_t queue_family : unique_queue_families) {
    VkDeviceQueueCreateInfo queue_create_info{};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = queue_family;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;
    queue_create_infos.push_back(queue_create_info);
  }

  log_all<log_event::message>("Creating Vulkan logical device");
  VkPhysicalDeviceFeatures device_features{};

  VkDeviceCreateInfo device_create_info{};
  device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

  device_create_info.queueCreateInfoCount =
      static_cast<std::uint32_t>(queue_create_infos.size());
  device_create_info.pQueueCreateInfos = queue_create_infos.data();

  device_create_info.pEnabledFeatures = &device_features;

  device_create_info.enabledExtensionCount = 0;

#ifdef SURGE_VULKAN_VALIDATION
  device_create_info.enabledLayerCount =
      static_cast<std::uint32_t>(required_vulkan_validation_layers.size());
  device_create_info.ppEnabledLayerNames =
      required_vulkan_validation_layers.data();
#else
  device_create_info.enabledLayerCount = 0;
#endif

  // TODO: Replace nullptr with allocator
  const auto status = vkCreateDevice(
      selected_physical_device, &device_create_info, nullptr, &logical_device);

  if (status != VK_SUCCESS) {
    log_all<log_event::message>(
        "Unable to initialize Vulkan logical device. Vulkan error code {}",
        status);
    return false;
  }

  log_all<log_event::message>("Vulkan logical device created.");
  logical_device_created = true;

  log_all<log_event::message>("Retrieving Vulkan graphics queue.");
  vkGetDeviceQueue(logical_device, indices.graphics_family.value(), 0,
                   &graphics_queue);

  log_all<log_event::message>("Retrieving Vulkan present queue.");
  vkGetDeviceQueue(logical_device, indices.present_family.value(), 0,
                   &present_queue);

  return true;
}

auto surge::global_vulkan_instance::create_surface(GLFWwindow *window) noexcept
    -> bool {

  log_all<log_event::message>("Creating Vulkan window surface");

  // TODO: Replace nullptr with allocators
  const auto status =
      glfwCreateWindowSurface(instance, window, nullptr, &window_surface);

  if (status != VK_SUCCESS) {
    log_all<log_event::message>(
        "Unable to initialize Vulkan window surface. Vulkan error code {}",
        status);
    return false;
  }

  log_all<log_event::message>("Vulkan window surface created.");
  surface_created = true;

  return true;
}