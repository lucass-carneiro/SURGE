#include "default.hpp"

#include <glm/fwd.hpp>

extern "C" {

SURGE_MODULE_EXPORT void draw(GLFWwindow *) noexcept {
  using namespace surge;
  using namespace globals;

  fonts::render_text(*freetype_ctx, *char_map, 0, glm::vec3{10.0f, 80.0f, 1.0f},
                     glm::vec3{220.0f / 256.0f, 20.0f / 256.0f, 60.0f / 256.0f}, "SURGE");
  fonts::render_text(*freetype_ctx, *char_map, 1, glm::vec3{10.0f, 130.0f, 0.3f},
                     glm::vec3{0.0f, 0.0f, 0.0f}, "The Super Underrated Game Engine");
  fonts::render_text(*freetype_ctx, *char_map, 1, glm::vec3{10.0f, 160.0f, 0.3f},
                     glm::vec3{0.0f, 0.0f, 0.0f}, "Created with love by the Ninja Sheep");

  sheep_dc.pos = glm::vec3{100.0f, 200.0f, 0.1};
  sheep_dc.scale = glm::vec3{102.0f, 102.0f, 1.0f};
  renderer::image::draw_region(*sheep_img, sheep_dc, glm::vec2{1.0f, 1.0f},
                               glm::vec2{102.0f, 102.0f});
}
}