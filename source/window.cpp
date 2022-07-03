#include "window.hpp"
#include "log.hpp"
#include "options.hpp"

#include <EASTL/vector.h>
#include <cstddef>
#include <tl/expected.hpp>

#include <iostream>

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
    : instance_created{false}, app_info{}, create_info{} {

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
  log_all<log_event::message>("Destroying Vulkan instance");
  if (instance_created) {
    // TODO: Replace nullptr with allocator callbacks
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
  }

  return instance_created;
}

auto surge::global_vulkan_instance::check_extensions() noexcept -> bool {
  // TODO: Check if required and available are compatible

  log_all<log_event::message>("Querying required Vulkan extensions");
  std::uint32_t required_extension_count = 0;
  const auto required_extensions =
      glfwGetRequiredInstanceExtensions(&required_extension_count);

  for (std::uint32_t i = 0; i < required_extension_count; i++) {
    log_all<log_event::message>("Extension {} required",
                                required_extensions[i]);
  }

  log_all<log_event::message>("Querrying available Vulkan extensions");
  std::uint32_t available_extension_count{0};
  vkEnumerateInstanceExtensionProperties(nullptr, &available_extension_count,
                                         nullptr);

  eastl::vector<VkExtensionProperties> available_extensions(
      available_extension_count);
  vkEnumerateInstanceExtensionProperties(nullptr, &available_extension_count,
                                         available_extensions.data());

  for (const auto &available_extension : available_extensions) {
    log_all<log_event::message>("Extension {} available",
                                available_extension.extensionName);
  }

  create_info.enabledExtensionCount = required_extension_count;
  create_info.ppEnabledExtensionNames = required_extensions;

  return true;
}

#ifdef SURGE_VULKAN_VALIDATION
auto surge::global_vulkan_instance::check_validation_layers() noexcept -> bool {
  // TODO: Check if required and available are compatible

  std::uint32_t available_layer_count{0};
  vkEnumerateInstanceLayerProperties(&available_layer_count, nullptr);

  eastl::vector<VkLayerProperties> available_layers(available_layer_count);
  vkEnumerateInstanceLayerProperties(&available_layer_count,
                                     available_layers.data());

  if (required_validation_layer_count > available_layer_count) {
    log_all<log_event::error>("There are {} available vulkan validation "
                              "layer(s) but {} layer(s) is(are) required",
                              available_layer_count,
                              required_validation_layer_count);
    return false;
  }

  for (const auto &layer : required_vulkan_validation_layers) {
    log_all<log_event::message>("Validation layer {} enabled", layer);
  }

  create_info.enabledLayerCount = required_validation_layer_count;
  create_info.ppEnabledLayerNames = required_vulkan_validation_layers.data();

  return true;
}
#endif