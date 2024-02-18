#ifndef SURGE_MODULE_DTU_DEBUG_WINDOW_HPP
#define SURGE_MODULE_DTU_DEBUG_WINDOW_HPP

// clang-format off
#include "DTU.hpp"
#include "commands.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
// clang-format on

namespace DTU::debug_window {

void init(GLFWwindow *window) noexcept;
void cleanup() noexcept;

void main_window(const DTU::vec_glui &ids, const vec_glui64 &handles, const cmdq_t &cmdq,
                 const sdl_t &sdl, const sdl_t &ui_sdl, bool *p_open = nullptr) noexcept;

void draw(const DTU::vec_glui &ids, const vec_glui64 &handles, const cmdq_t &cmdq, const sdl_t &sdl,
          const sdl_t &ui_sdl) noexcept;

} // namespace DTU::debug_window

#endif // SURGE_MODULE_DTU_DEBUG_WINDOW_HPP