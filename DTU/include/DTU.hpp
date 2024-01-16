#ifndef SURGE_MODULE_DTU_HPP
#define SURGE_MODULE_DTU_HPP

#include "container_types.hpp"
#include "integer_types.hpp"

// clang-format off
#include "player/allocators.hpp"
#include "player/options.hpp"
#include "player/static_image.hpp"
#include "player/window.hpp"
#include "static_image.hpp"
// clang-format on

#include <foonathan/memory/container.hpp>

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

// Global accessors
auto get_projection() noexcept -> const glm::mat4 &;
auto get_view() noexcept -> glm::mat4 &;
auto get_img_shader() noexcept -> GLuint;

namespace components {

namespace static_image_buffer {

auto dimentions() noexcept -> DTU::vector<glm::vec2> &;
auto ds() noexcept -> DTU::vector<glm::vec2> &;
auto texture_id() noexcept -> DTU::vector<GLuint> &;
auto VBO() noexcept -> DTU::vector<GLuint> &;
auto EBO() noexcept -> DTU::vector<GLuint> &;
auto VAO() noexcept -> DTU::vector<GLuint> &;

void clean_and_reset() noexcept;

} // namespace static_image_buffer

namespace static_image_draw_noflip {

auto pos() noexcept -> DTU::vector<glm::vec3> &;
auto scale() noexcept -> DTU::vector<glm::vec3> &;
auto region_origin() noexcept -> DTU::vector<glm::vec2> &;
auto region_dims() noexcept -> DTU::vector<glm::vec2> &;

} // namespace static_image_draw_noflip

} // namespace components

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