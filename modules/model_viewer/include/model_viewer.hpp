#ifndef SURGE_MODULE_MODEL_VIEWER
#define SURGE_MODULE_MODEL_VIEWER

#include "player/options.hpp"
#include "player/window.hpp"

#if defined(SURGE_COMPILER_Clang)                                                                  \
    || defined(SURGE_COMPILER_GCC) && COMPILING_SURGE_MODULE_MODEL_VIEWER
#  define SURGE_MODULE_EXPORT __attribute__((__visibility__("default")))
#elif defined(SURGE_COMPILER_MSVC) && COMPILING_SURGE_MODULE_MODEL_VIEWER
#  define SURGE_MODULE_EXPORT __declspec(dllexport)
#elif defined(SURGE_COMPILER_MSVC)
#  define SURGE_MODULE_EXPORT __declspec(dllimport)
#else
#  define SURGE_MODULE_EXPORT
#endif

namespace mod_model_viewer {

auto bind_callbacks(GLFWwindow *window) noexcept -> int;
auto unbind_callbacks(GLFWwindow *window) noexcept -> int;

} // namespace mod_model_viewer

extern "C" {
SURGE_MODULE_EXPORT auto on_load(GLFWwindow *window) noexcept -> int;
SURGE_MODULE_EXPORT auto on_unload(GLFWwindow *window) noexcept -> int;
SURGE_MODULE_EXPORT auto draw() noexcept -> int;
SURGE_MODULE_EXPORT auto update(double dt) noexcept -> int;

SURGE_MODULE_EXPORT void keyboard_event(GLFWwindow *window, int key, int scancode, int action,
                                        int mods) noexcept;

SURGE_MODULE_EXPORT void mouse_button_event(GLFWwindow *window, int button, int action,
                                            int mods) noexcept;

SURGE_MODULE_EXPORT void mouse_scroll_event(GLFWwindow *window, double xoffset,
                                            double yoffset) noexcept;
}

#endif // SURGE_MODULE_MODEL_VIEWER