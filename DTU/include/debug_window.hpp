#ifndef SURGE_MODULE_DTU_DEBUG_WINDOW_HPP
#define SURGE_MODULE_DTU_DEBUG_WINDOW_HPP

#include <player/container_types.hpp>
#include <player/sprite.hpp>

// clang-format off
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
// clang-format on

namespace DTU::debug_window {

void init(GLFWwindow *window) noexcept;
void cleanup() noexcept;

void main_window(const surge::deque<surge::u32> &cmdq, const surge::atom::sprite::data_list &sdl,
                 bool *p_open = nullptr) noexcept;

void draw(const surge::deque<surge::u32> &cmdq, const surge::atom::sprite::data_list &sdl) noexcept;

} // namespace DTU::debug_window

#endif // SURGE_MODULE_DTU_DEBUG_WINDOW_HPP