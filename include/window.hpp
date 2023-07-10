#ifndef SURGE_WINDOW_HPP
#define SURGE_WINDOW_HPP

#include "options.hpp"

// clang-format off
#include "lua/lua_bindings.hpp"
#include "lua/lua_states.hpp"
#include "lua/lua_utils.hpp"
#include "lua/lua_wrappers.hpp"

#include "opengl/headers.hpp"
// clang-format on

#ifdef SURGE_ENABLE_TRACY
#  include <tracy/Tracy.hpp>
#endif

#include <algorithm>
#include <cstddef>
#include <glm/mat4x4.hpp>
#include <gsl/gsl-lite.hpp>
#include <memory>
#include <optional>
#include <tuple>

namespace surge {

namespace default_shaders {

extern GLuint sprite_shader;
extern GLuint image_shader;

} // namespace default_shaders

namespace engine_window {

using window_ptr_t = std::unique_ptr<GLFWwindow, void (*)(GLFWwindow *)>;

extern window_ptr_t window;

auto init(const lua_engine_config &engine_config) noexcept -> bool;

void glfw_error_callback(int code, const char *description) noexcept;
void framebuffer_size_callback(GLFWwindow *, int width, int height) noexcept;
void glfw_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) noexcept;
void glfw_mouse_button_callback(GLFWwindow *window, int button, int action, int mods) noexcept;
void glfw_scroll_callback(GLFWwindow *window, double xoffset, double yoffset) noexcept;

} // namespace engine_window

} // namespace surge

#endif // SURGE_WINDOW_HPP