#ifndef SURGE_MODULE_DTU_HPP
#define SURGE_MODULE_DTU_HPP

// clang-format off
#include "player/options.hpp"
#include "player/container_types.hpp"
#include "player/static_image.hpp"
#include "player/window.hpp"
#include "static_image.hpp"
// clang-format on

#include <optional>

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
auto bind_callbacks(GLFWwindow *window) noexcept -> int;
auto unbind_callbacks(GLFWwindow *window) noexcept -> int;

#ifdef SURGE_BUILD_TYPE_Debug
auto get_command_queue() noexcept -> const surge::deque<surge::u32> &;
#endif

namespace sprite {

// Data for all loaded sprites. Loading and unloading appends to instances of this type
struct data_list {
  surge::vector<GLuint> texture_ids;
  surge::vector<GLuint64> texture_handles;
  surge::vector<glm::mat4> models;
  surge::vector<float> alphas;
};

} // namespace sprite

} // namespace DTU

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

#endif // SURGE_MODULE_DTU_HPP