#ifndef SURGE_MODULE_DTU_HPP
#define SURGE_MODULE_DTU_HPP

// clang-format off
#include "player/options.hpp"
#include "player/container_types.hpp"
#include "player/window.hpp"
#include "player/sprite.hpp"
#include "player/text.hpp"
// clang-format on

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

using vec_glui = surge::vector<GLuint>;
using vec_glui64 = surge::vector<GLuint64>;
using sdl_t = surge::atom::sprite::data_list;
using tdd_t = surge::atom::text::text_draw_data;
using tgd_t = surge::atom::text::glyph_data;
using sbd_t = surge::atom::sprite::buffer_data;
using tbd_t = surge::atom::text::buffer_data;

// Callbacks
auto bind_callbacks(GLFWwindow *window) noexcept -> int;
auto unbind_callbacks(GLFWwindow *window) noexcept -> int;

auto load_texture(vec_glui &ids, vec_glui64 &handles, const char *img_path,
                  surge::renderer::texture_filtering filtering
                  = surge::renderer::texture_filtering::nearest) noexcept -> GLuint64;
void unload_textures(vec_glui &ids, vec_glui64 &handles) noexcept;

void push_sprite(sdl_t &sdl, GLuint64 handle, glm::mat4 &&model, float alpha) noexcept;

void clear_sprites(sdl_t &sdl) noexcept;
void clear_text(tdd_t &tdd) noexcept;

auto load_push_sprite(vec_glui &ids, vec_glui64 &handles, const char *img_path, sdl_t &sdl,
                      glm::mat4 &&model, float alpha) noexcept -> GLuint64;

auto make_model(const glm::vec3 &pos, const glm::vec3 &scale) noexcept -> glm::mat4;

} // namespace DTU

extern "C" {
SURGE_MODULE_EXPORT auto on_load(GLFWwindow *window) noexcept -> int;
SURGE_MODULE_EXPORT auto on_unload(GLFWwindow *window) noexcept -> int;
SURGE_MODULE_EXPORT auto draw() noexcept -> int;
SURGE_MODULE_EXPORT auto update(GLFWwindow *window, double dt) noexcept -> int;

SURGE_MODULE_EXPORT void keyboard_event(GLFWwindow *window, int key, int scancode, int action,
                                        int mods) noexcept;

SURGE_MODULE_EXPORT void mouse_button_event(GLFWwindow *window, int button, int action,
                                            int mods) noexcept;

SURGE_MODULE_EXPORT void mouse_scroll_event(GLFWwindow *window, double xoffset,
                                            double yoffset) noexcept;
}

#endif // SURGE_MODULE_DTU_HPP