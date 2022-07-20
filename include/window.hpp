#ifndef SURGE_WINDOW_HPP
#define SURGE_WINDOW_HPP

#include "arena_allocator.hpp"
#include "options.hpp"

// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <EASTL/vector.h>
// clang-format on

#include <cstddef>
#include <memory>
#include <optional>
#include <utility>

namespace surge {

void glfw_error_callback(int code, const char *description) noexcept;

void framebuffer_size_callback(GLFWwindow *, int width, int height) noexcept;

/**
 * Querry the existing available monitors.
 *
 * @return The monitor array if the monitors could or false.
 */
auto querry_available_monitors() noexcept
    -> std::optional<std::pair<GLFWmonitor **, std::size_t>>;

} // namespace surge

#endif // SURGE_WINDOW_HPP