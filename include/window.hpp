#ifndef SURGE_WINDOW_HPP
#define SURGE_WINDOW_HPP

// clang-format off
#include <cstddef>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
// clang-format on

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

} // namespace surge

#endif // SURGE_WINDOW_HPP