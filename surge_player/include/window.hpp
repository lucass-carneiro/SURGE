#ifndef SURGE_WINDOW_HPP
#define SURGE_WINDOW_HPP

#include <GLFW/glfw3.h>
#include <bgfx/bgfx.h>
#include <cstdint>
#include <tuple>

namespace surge::window {

auto init(const char *config_file) noexcept
    -> std::tuple<GLFWwindow *, std::uint32_t, std::uint32_t, std::uint32_t>;
void terminate(GLFWwindow *window) noexcept;

void handle_resize(GLFWwindow *window, std::uint32_t &old_w, std::uint32_t &old_h,
                   std::uint32_t reset_flags) noexcept;

} // namespace surge::window

#endif // SURGE_WINDOW_HPP