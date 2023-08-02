#ifndef SURGE_WINDOW_HPP
#define SURGE_WINDOW_HPP

#include "renderer.hpp"

#include <cstdint>
#include <tuple>

namespace surge::window {

auto init(const char *config_file) noexcept
    -> std::tuple<GLFWwindow *, std::uint32_t, std::uint32_t, renderer::clear_color>;
void terminate(GLFWwindow *window) noexcept;

auto get_dims(GLFWwindow *window) noexcept -> std::tuple<float, float>;

} // namespace surge::window

#endif // SURGE_WINDOW_HPP