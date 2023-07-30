#ifndef SURGE_MODULE_MANAGER_HPP
#define SURGE_MODULE_MANAGER_HPP

// clang-format off
#include <GLFW/glfw3.h>
// clang-format on

#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace surge::module {

using handle_t = void *;

using on_load_fun = void (*)();
using update_fun = void (*)(double);
using draw_fun = void (*)();

auto load(const char *module_name) noexcept -> handle_t;
void unload(handle_t module_handle) noexcept;
auto reload(GLFWwindow *window, handle_t module_handle) noexcept -> handle_t;

void on_load(GLFWwindow *window, handle_t module_handle);
void on_unload(GLFWwindow *window, handle_t module_handle);

void update(handle_t module_handle, double dt) noexcept;
void draw(handle_t module_handle) noexcept;

} // namespace surge::module

#endif // SURGE_MODULE_MANAGER_HPP