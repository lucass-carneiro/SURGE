#ifndef SURGE_MODULE_2048
#define SURGE_MODULE_2048

#include "allocators.hpp"
#include "options.hpp"
#include "static_image.hpp"
#include "text.hpp"
#include "window.hpp"

#include <EASTL/deque.h>
#include <cstdint>
#include <glm/fwd.hpp>

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
  img_shader_load,
  txt_shader_load,
  board_img_load,
  pieces_img_load,
  numbers_img_load,
  fonts_load,
  charmap_create
};

using state_code_t = unsigned short;
enum class game_state : state_code_t {
  idle = 0,
  compress_up = 1,
  compress_down = 2,
  compress_left = 3,
  compress_right = 4,
  merge_up = 5,
  merge_down = 6,
  merge_left = 7,
  merge_right = 8,
  piece_removal = 9,
  add_piece = 10
};

using state_queue = eastl::deque<game_state, surge::allocators::eastl::gp_allocator>;

auto bind_callbacks(GLFWwindow *window) noexcept -> std::uint32_t;
auto unbind_callbacks(GLFWwindow *window) noexcept -> std::uint32_t;

auto get_projection() noexcept -> const glm::mat4 &;
auto get_view() noexcept -> const glm::mat4 &;

using buffer_t = surge::atom::static_image::one_buffer_data;
using draw_t = surge::atom::static_image::one_draw_data;

auto get_board_buffer() noexcept -> const buffer_t &;
auto get_pieces_buffer() noexcept -> const buffer_t &;

using text_buffer_t = surge::atom::text::buffer_data;
using text_charmap_t = surge::atom::text::charmap_data;
auto get_text_buffer() noexcept -> const text_buffer_t &;
auto get_text_charmap() noexcept -> const text_charmap_t &;

auto get_img_shader() noexcept -> GLuint;
auto get_txt_shader() noexcept -> GLuint;

auto get_board_draw_data() noexcept -> const draw_t &;

using texture_origins_t = const std::array<glm::vec2, 11>;

auto get_piece_texture_origins() noexcept -> texture_origins_t &;

using slot_coords_t = const std::array<glm::vec3, 16>;

using points_t = unsigned long long;

auto get_slot_coords() noexcept -> const slot_coords_t &;

auto get_slot_size() noexcept -> float;

auto get_slot_delta() noexcept -> float;

auto inside_new_game_button(double x, double y) noexcept -> bool;
void new_game() noexcept;

auto view_state_queue() noexcept -> const state_queue &;
auto get_state_queue() noexcept -> state_queue &;

auto get_game_score() noexcept -> points_t &;
void add_game_score(points_t points) noexcept;
auto get_best_score() noexcept -> points_t &;

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

#endif // SURGE_MODULE_2048
