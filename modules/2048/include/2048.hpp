#ifndef SURGE_MODULE_2048_HPP
#define SURGE_MODULE_2048_HPP

#include "options.hpp"
#include "window.hpp"

#if defined(SURGE_COMPILER_Clang) || defined(SURGE_COMPILER_GCC) && COMPILING_SURGE_MODULE_2048
#  define SURGE_MODULE_EXPORT __attribute__((__visibility__("default")))
#elif defined(SURGE_COMPILER_MSVC) && COMPILING_SURGE_MODULE_2048
#  define SURGE_MODULE_EXPORT __declspec(dllexport)
#elif defined(SURGE_COMPILER_MSVC)
#  define SURGE_MODULE_EXPORT __declspec(dllimport)
#else
#  define SURGE_MODULE_EXPORT
#endif

namespace mod_2048 {

enum class error : std::uint32_t {
  keyboard_event_binding,
  mouse_button_event_binding,
  mouse_scroll_event_binding,
  keyboard_event_unbinding,
  mouse_button_event_unbinding,
  mouse_scroll_event_unbinding,
  board_image_context,
  piece_image_context
};

auto bind_callbacks(GLFWwindow *window) noexcept -> std::uint32_t;
auto unbind_callbacks(GLFWwindow *window) noexcept -> std::uint32_t;

} // namespace mod_2048

extern "C" {

SURGE_MODULE_EXPORT auto on_load(GLFWwindow *window) noexcept -> std::uint32_t;
SURGE_MODULE_EXPORT auto on_unload(GLFWwindow *window) noexcept -> std::uint32_t;
SURGE_MODULE_EXPORT auto draw() noexcept -> std::uint32_t;
SURGE_MODULE_EXPORT auto update(double dt) noexcept -> std::uint32_t;

SURGE_MODULE_EXPORT void keyboard_event(GLFWwindow *window, int key, int scancode, int action,
                                        int mods) noexcept;

SURGE_MODULE_EXPORT void mouse_button_event(GLFWwindow *window, int button, int action,
                                            int mods) noexcept;

SURGE_MODULE_EXPORT void mouse_scroll_event(GLFWwindow *window, double xoffset,
                                            double yoffset) noexcept;
}

#endif // SURGE_MODULE_2048_HPP