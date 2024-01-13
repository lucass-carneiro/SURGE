#ifndef SURGE_MODULE_DTU_HPP
#define SURGE_MODULE_DTU_HPP

#include "integer_types.hpp"
#include "player/options.hpp"
#include "player/window.hpp"

#if defined(SURGE_COMPILER_Clang) || defined(SURGE_COMPILER_GCC) && COMPILING_SURGE_MODULE_DEFAULT
#  define SURGE_MODULE_EXPORT __attribute__((__visibility__("default")))
#elif defined(SURGE_COMPILER_MSVC) && COMPILING_SURGE_MODULE_DEFAULT
#  define SURGE_MODULE_EXPORT __declspec(dllexport)
#elif defined(SURGE_COMPILER_MSVC)
#  define SURGE_MODULE_EXPORT __declspec(dllimport)
#else
#  define SURGE_MODULE_EXPORT
#endif

namespace DTU {

// Callbacks
auto bind_callbacks(GLFWwindow *window) noexcept -> u32;
auto unbind_callbacks(GLFWwindow *window) noexcept -> u32;

} // namespace DTU

extern "C" {
SURGE_MODULE_EXPORT auto on_load(GLFWwindow *window) noexcept -> DTU::u32;
SURGE_MODULE_EXPORT auto on_unload(GLFWwindow *window) noexcept -> DTU::u32;
SURGE_MODULE_EXPORT auto draw() noexcept -> DTU::u32;
SURGE_MODULE_EXPORT auto update(double dt) noexcept -> DTU::u32;

SURGE_MODULE_EXPORT void keyboard_event(GLFWwindow *window, int key, int scancode, int action,
                                        int mods) noexcept;

SURGE_MODULE_EXPORT void mouse_button_event(GLFWwindow *window, int button, int action,
                                            int mods) noexcept;

SURGE_MODULE_EXPORT void mouse_scroll_event(GLFWwindow *window, double xoffset,
                                            double yoffset) noexcept;
}

#endif // SURGE_MODULE_DTU_HPP