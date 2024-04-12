#ifndef SURGE_MODULE_DTU_DEBUG_WINDOW_HPP
#define SURGE_MODULE_DTU_DEBUG_WINDOW_HPP

#include "player/window.hpp"

// clang-format off
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
// clang-format on

namespace DTU::debug_window {

void create(GLFWwindow *window) noexcept;
void destroy() noexcept;

void main_window(const std::tuple<float, float> &wdims) noexcept;

void draw(const std::tuple<float, float> &wdims, bool &show) noexcept;

} // namespace DTU::debug_window

#endif // SURGE_MODULE_DTU_DEBUG_WINDOW_HPP