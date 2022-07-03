#ifndef SURGE_WINDOW_HPP
#define SURGE_WINDOW_HPP

#include "options.hpp"

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

#ifdef SURGE_VULKAN_VALIDATION
  static constexpr const std::uint32_t required_validation_layer_count =
      std::uint32_t{required_vulkan_validation_layers.size()};
#endif

  auto create_instance() noexcept -> bool;

  auto check_extensions() noexcept -> bool;

#ifdef SURGE_VULKAN_VALIDATION
  auto check_validation_layers() noexcept -> bool;
#endif

  ~global_vulkan_instance() noexcept;

private:
  global_vulkan_instance() noexcept;

  bool instance_created;

  VkApplicationInfo app_info;
  VkInstanceCreateInfo create_info;

  VkInstance instance;
};
} // namespace surge

#endif // SURGE_WINDOW_HPP