#ifndef SURGE_MODULE_DEFAULT
#define SURGE_MODULE_DEFAULT

#include "font_cache.hpp"
#include "options.hpp"
#include "window.hpp"

#if defined(SURGE_COMPILER_Clang) || defined(SURGE_COMPILER_GCC) && COMPILING_SURGE_MODULE_DEFAULT
#  define SURGE_MODULE_EXPORT __attribute__((__visibility__("default")))
#elif defined(SURGE_COMPILER_MSVC) && COMPILING_SURGE_MODULE_DEFAULT
#  define SURGE_MODULE_EXPORT __declspec(dllexport)
#elif defined(SURGE_COMPILER_MSVC)
#  define SURGE_MODULE_EXPORT __declspec(dllimport)
#else
#  define SURGE_MODULE_EXPORT
#endif

namespace globals {

extern std::optional<surge::fonts::font_system_context> freetype_ctx;
extern std::optional<surge::fonts::charmap> char_map;

extern std::optional<surge::renderer::image::context> sheep_img;
extern surge::renderer::image::draw_context sheep_dc;

}; // namespace globals

extern "C" {

SURGE_MODULE_EXPORT auto on_load(GLFWwindow *window) noexcept -> bool;
SURGE_MODULE_EXPORT void on_unload() noexcept;

SURGE_MODULE_EXPORT void draw(GLFWwindow *window) noexcept;
SURGE_MODULE_EXPORT void update(double dt) noexcept;

SURGE_MODULE_EXPORT void keyboard_event(GLFWwindow *window, int key, int scancode, int action,
                                        int mods) noexcept;

SURGE_MODULE_EXPORT void mouse_button_event(GLFWwindow *window, int button, int action,

                                            int mods) noexcept;
SURGE_MODULE_EXPORT void mouse_scroll_event(GLFWwindow *window, double xoffset,
                                            double yoffset) noexcept;
}

#endif // SURGE_MODULE_DEFAULT