#ifndef SURGE_WINDOW_HPP
#define SURGE_WINDOW_HPP

#include "arena_allocator.hpp"
#include "options.hpp"
#include <vulkan/vulkan_core.h>

// clang-format off
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <EASTL/vector.h>
// clang-format on

#include <cstddef>
#include <memory>
#include <optional>
#include <utility>

namespace surge {

void glfw_error_callback(int code, const char *description) noexcept;

/**
 * Querry the existing available monitors.
 *
 * @return The monitor array if the monitors could or false.
 */
auto querry_available_monitors() noexcept
    -> std::optional<std::pair<GLFWmonitor **, std::size_t>>;

/**
 * Holds and initializes the vulkan instance
 */
class global_vulkan_instance {
public:
  static auto get() noexcept -> global_vulkan_instance & {
    static global_vulkan_instance gvi;
    return gvi;
  }

  static const char *const application_name;

  auto create_instance() noexcept -> bool;

  auto check_extensions() noexcept -> bool;

#ifdef SURGE_VULKAN_VALIDATION
  auto check_validation_layers() noexcept -> bool;
#endif

  auto pick_physical_device() noexcept -> bool;

  ~global_vulkan_instance() noexcept;

private:
  global_vulkan_instance() noexcept;

  VKAPI_ATTR auto VKAPI_CALL
  debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                 VkDebugUtilsMessageTypeFlagsEXT message_type,
                 const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
                 void *user_data) -> VkBool32;

  bool instance_created;

  VkApplicationInfo app_info;
  VkInstanceCreateInfo create_info;

  eastl::vector<VkExtensionProperties> available_extensions;
  eastl::vector<const char *> required_extensions;

  VkInstance instance = VK_NULL_HANDLE;

  eastl::vector<VkPhysicalDevice> available_physical_devices;
  VkPhysicalDevice selected_physical_device = VK_NULL_HANDLE;

  eastl::vector<VkQueueFamilyProperties> available_queue_families;

#ifdef SURGE_VULKAN_VALIDATION
  eastl::vector<VkLayerProperties> available_layers;
#endif

  void get_required_extensions() noexcept;
  void get_available_extensions() noexcept;

#ifdef SURGE_VULKAN_VALIDATION
  void get_available_validation_layers() noexcept;
#endif

  void get_available_physical_devices() noexcept;

  auto is_suitable(VkPhysicalDevice device) noexcept -> bool;

  void print_device_summary(VkPhysicalDevice device) noexcept;

  [[nodiscard]] auto device_type_string(std::uint8_t id) const noexcept -> const
      char *;

  struct queue_family_indices {
    std::optional<std::uint32_t> graphics_family;

    [[nodiscard]] inline auto is_complete() const noexcept -> bool {
      return graphics_family.has_value();
    }
  };

  void get_available_queue_families(VkPhysicalDevice device) noexcept;

  auto find_queue_families(VkPhysicalDevice device) noexcept
      -> queue_family_indices;
};

} // namespace surge

#endif // SURGE_WINDOW_HPP