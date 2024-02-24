#include "ui.hpp"

auto DTU::ui::spinner_box(const glm::vec2 &, const glm::vec3 &draw_pos, const GLuint64 &skin_handle,
                          sdl_t &sdl, std::string_view, bool &) noexcept -> bool {
  // TODO
  static bool once{false};
  if (!once) {
    push_sprite(sdl, skin_handle, make_model(draw_pos, glm::vec3{100.0f}), 1.0f);
    once = true;
  }
  return true;
}