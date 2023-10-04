#ifndef SURGE_MODULE_LASERS
#define SURGE_MODULE_LASERS

#include "logging.hpp"
#include "options.hpp"
#include "renderer.hpp"
#include "window.hpp"

#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#if defined(SURGE_COMPILER_Clang) || defined(SURGE_COMPILER_GCC) && COMPILING_SURGE_MODULE_LASERS
#  define SURGE_MODULE_EXPORT __attribute__((__visibility__("default")))
#elif defined(SURGE_COMPILER_MSVC) && COMPILING_SURGE_MODULE_LASERS
#  define SURGE_MODULE_EXPORT __declspec(dllexport)
#elif defined(SURGE_COMPILER_MSVC)
#  define SURGE_MODULE_EXPORT __declspec(dllimport)
#else
#  define SURGE_MODULE_EXPORT
#endif

namespace surge::mod::lasers {

enum class error : std::uint32_t {
  keyboard_event_binding,
  mouse_button_event_binding,
  mouse_scroll_event_binding,
  keyboard_event_unbinding,
  mouse_button_event_unbinding,
  mouse_scroll_event_unbinding,
  shader_creation,
};

auto get_global_projection() noexcept -> const glm::mat4 &;
auto get_global_view() noexcept -> const glm::mat4 &;

auto get_global_smo_shader() noexcept -> const GLuint &;

auto get_global_cell_grid_ctx() noexcept -> const surge::renderer::smo::context &;

auto bind_callbacks(GLFWwindow *window) noexcept -> std::uint32_t;
auto unbind_callbacks(GLFWwindow *window) noexcept -> std::uint32_t;

} // namespace surge::mod::lasers

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

#endif // SURGE_MODULE_LASERS